# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module encapsulating Zmake wrapper object."""
import difflib
import logging
import os
import pathlib
import re
import shutil
import subprocess
import tempfile

import zmake.build_config
import zmake.generate_readme
import zmake.jobserver
import zmake.modules
import zmake.multiproc
import zmake.project
import zmake.util as util
import zmake.version

ninja_warnings = re.compile(r"^(\S*: )?warning:.*")
ninja_errors = re.compile(r"error:.*")


def ninja_stdout_log_level_override(line, current_log_level):
    """Update the log level for ninja builds if we hit an error.

    Ninja builds prints everything to stdout, but really we want to start
    logging things to CRITICAL

    Args:
        line: The line that is about to be logged.
        current_log_level: The active logging level that would be used for the
          line.
    """
    # Output lines from Zephyr that are not normally useful
    # Send any lines that start with these strings to INFO
    cmake_suppress = [
        "-- ",  # device tree messages
        "Loaded configuration",
        "Including boilerplate",
        "Parsing ",
        "No change to configuration",
        "No change to Kconfig header",
    ]

    # Herewith a long list of things which are really for debugging, not
    # development. Return logging.DEBUG for each of these.

    # ninja puts progress information on stdout
    if line.startswith("["):
        return logging.DEBUG
    # we don't care about entering directories since it happens every time
    if line.startswith("ninja: Entering directory"):
        return logging.DEBUG
    # we know the build stops from the compiler messages and ninja return code
    if line.startswith("ninja: build stopped"):
        return logging.DEBUG
    # someone prints a *** SUCCESS *** message which we don't need
    if line.startswith("***"):
        return logging.DEBUG
    # dopey ninja puts errors on stdout, so fix that. It does not look
    # likely that it will be fixed upstream:
    # https://github.com/ninja-build/ninja/issues/1537
    # Try to drop output about the device tree
    if any(line.startswith(x) for x in cmake_suppress):
        return logging.INFO
    # this message is a bit like make failing. We already got the error output.
    if line.startswith("FAILED: CMakeFiles"):
        return logging.INFO
    # if a particular file fails it shows the build line used, but that is not
    # useful except for debugging.
    if line.startswith("ccache"):
        return logging.DEBUG
    if ninja_warnings.match(line):
        return logging.WARNING
    if ninja_errors.match(line):
        return logging.ERROR
    # When we see "Memory region" go into INFO, and stay there as long as the
    # line starts with \S+:
    if line.startswith("Memory region"):
        return logging.INFO
    if current_log_level == logging.INFO and line.split()[0].endswith(":"):
        return current_log_level
    if current_log_level == logging.WARNING:
        return current_log_level
    return logging.ERROR


def cmake_log_level_override(line, default_log_level):
    """Update the log level for cmake output if we hit an error.

    Cmake prints some messages that are less than useful during
    development.

    Args:
        line: The line that is about to be logged.
        default_log_level: The default logging level that will be used for the
          line.
    """
    # Strange output from Zephyr that we normally ignore
    if line.startswith("Including boilerplate"):
        return logging.DEBUG
    elif line.startswith("devicetree error:"):
        return logging.ERROR
    if ninja_warnings.match(line):
        return logging.WARNING
    if ninja_errors.match(line):
        return logging.ERROR
    return default_log_level


def get_process_failure_msg(proc):
    """Creates a suitable failure message if something exits badly

    Args:
        proc: subprocess.Popen object containing the thing that failed

    Returns:
        Failure message as a string:
    """
    return "Execution failed (return code={}): {}\n".format(
        proc.returncode, util.repr_command(proc.args)
    )


class Zmake:
    """Wrapper class encapsulating zmake's supported operations.

    The invocations of the constructor and the methods actually comes
    from the main function.  The command line arguments are translated
    such that dashes are replaced with underscores and applied as
    keyword arguments to the constructor and the method, and the
    subcommand invoked becomes the method run.

    As such, you won't find documentation for each method's parameters
    here, as it would be duplicate of the help strings from the
    command line.  Run "zmake --help" for full documentation of each
    parameter.

    Properties:
        executor: a zmake.multiproc.Executor object for submitting
            tasks to.
        _sequential: True to check the results of each build job sequentially,
            before launching more, False to just do this after all jobs complete
    """

    def __init__(
        self,
        checkout=None,
        jobserver=None,
        jobs=0,
        modules_dir=None,
        zephyr_base=None,
    ):
        zmake.multiproc.reset()
        self._checkout = checkout
        if zephyr_base:
            self.zephyr_base = zephyr_base
        else:
            self.zephyr_base = self.checkout / "src" / "third_party" / "zephyr" / "main"

        if modules_dir:
            self.module_paths = zmake.modules.locate_from_directory(modules_dir)
        else:
            self.module_paths = zmake.modules.locate_from_checkout(self.checkout)

        if jobserver:
            self.jobserver = jobserver
        else:
            try:
                self.jobserver = zmake.jobserver.GNUMakeJobClient.from_environ()
            except OSError:
                self.jobserver = zmake.jobserver.GNUMakeJobServer(jobs=jobs)

        self.logger = logging.getLogger(self.__class__.__name__)
        self.executor = zmake.multiproc.Executor()
        self._sequential = jobs == 1

    @property
    def checkout(self):
        if not self._checkout:
            self._checkout = util.locate_cros_checkout()
        return self._checkout.resolve()

    def configure(
        self,
        project_name_or_dir,
        build_dir=None,
        toolchain=None,
        build_after_configure=False,
        test_after_configure=False,
        bringup=False,
        coverage=False,
        allow_warnings=False,
    ):
        """Locate a project by name or directory and then call _configure."""
        root_dir = pathlib.Path(project_name_or_dir)
        if not root_dir.is_dir():
            root_dir = self.module_paths["ec"] / "zephyr"
        found_projects = zmake.project.find_projects(root_dir)
        if len(found_projects) == 1:
            # Likely passed directory path, wants to build only
            # project from there.
            project = next(iter(found_projects.values()))
        else:
            try:
                project = found_projects[project_name_or_dir]
            except KeyError as e:
                raise KeyError("No project named {}".format(project_name_or_dir)) from e
        return self._configure(
            project=project,
            build_dir=build_dir,
            toolchain=toolchain,
            build_after_configure=build_after_configure,
            test_after_configure=test_after_configure,
            bringup=bringup,
            coverage=coverage,
            allow_warnings=allow_warnings,
        )

    def _configure(
        self,
        project,
        build_dir=None,
        toolchain=None,
        build_after_configure=False,
        test_after_configure=False,
        bringup=False,
        coverage=False,
        allow_warnings=False,
    ):
        """Set up a build directory to later be built by "zmake build"."""
        # Resolve build_dir if needed.
        if not build_dir:
            build_dir = (
                self.module_paths["ec"]
                / "build"
                / "zephyr"
                / project.config.project_name
            )
        # Make sure the build directory is clean.
        if os.path.exists(build_dir):
            self.logger.info("Clearing old build directory %s", build_dir)
            shutil.rmtree(build_dir)

        generated_include_dir = (build_dir / "include").resolve()
        base_config = zmake.build_config.BuildConfig(
            environ_defs={"ZEPHYR_BASE": str(self.zephyr_base), "PATH": "/usr/bin"},
            cmake_defs={
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                "DTS_ROOT": str(self.module_paths["ec"] / "zephyr"),
                "SYSCALL_INCLUDE_DIRS": str(
                    self.module_paths["ec"] / "zephyr" / "include" / "drivers"
                ),
                "ZMAKE_INCLUDE_DIR": str(generated_include_dir),
            },
        )

        # Prune the module paths to just those required by the project.
        module_paths = project.prune_modules(self.module_paths)

        module_config = zmake.modules.setup_module_symlinks(
            build_dir / "modules", module_paths
        )

        # Symlink the Zephyr base into the build directory so it can
        # be used in the build phase.
        util.update_symlink(self.zephyr_base, build_dir / "zephyr_base")

        dts_overlay_config = project.find_dts_overlays(module_paths)

        toolchain_support = project.get_toolchain(module_paths, override=toolchain)
        toolchain_config = toolchain_support.get_build_config()

        if bringup:
            base_config |= zmake.build_config.BuildConfig(
                kconfig_defs={"CONFIG_PLATFORM_EC_BRINGUP": "y"}
            )
        if coverage:
            base_config |= zmake.build_config.BuildConfig(
                kconfig_defs={"CONFIG_COVERAGE": "y"}
            )
        if allow_warnings:
            base_config |= zmake.build_config.BuildConfig(
                cmake_defs={"ALLOW_WARNINGS": "ON"}
            )

        if not build_dir.exists():
            build_dir = build_dir.mkdir()
        if not generated_include_dir.exists():
            generated_include_dir.mkdir()
        processes = []
        self.logger.info("Building %s in %s.", project.config.project_name, build_dir)
        for build_name, build_config in project.iter_builds():
            self.logger.info(
                "Configuring %s:%s.", project.config.project_name, build_name
            )
            config = (
                base_config
                | toolchain_config
                | module_config
                | dts_overlay_config
                | build_config
            )
            output_dir = build_dir / "build-{}".format(build_name)
            kconfig_file = build_dir / "kconfig-{}.conf".format(build_name)
            proc = config.popen_cmake(
                self.jobserver,
                project.config.project_dir,
                output_dir,
                kconfig_file,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            job_id = "{}:{}".format(project.config.project_name, build_name)
            zmake.multiproc.log_output(
                self.logger,
                logging.DEBUG,
                proc.stdout,
                log_level_override_func=cmake_log_level_override,
                job_id=job_id,
            )
            zmake.multiproc.log_output(
                self.logger,
                logging.ERROR,
                proc.stderr,
                log_level_override_func=cmake_log_level_override,
                job_id=job_id,
            )
            if self._sequential:
                if proc.wait():
                    raise OSError(get_process_failure_msg(proc))
            else:
                processes.append(proc)
        for proc in processes:
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))

        # To reconstruct a Project object later, we need to know the
        # name and project directory.
        (build_dir / "project_name.txt").write_text(project.config.project_name)
        util.update_symlink(project.config.project_dir, build_dir / "project")

        if test_after_configure:
            rv = self.test(build_dir=build_dir)
            if rv or not coverage:
                return rv
            return self._coverage_run_test(
                project=project,
                build_dir=build_dir,
                lcov_file=build_dir / "output" / "zephyr.info",
                is_configured=True,
            )
        elif build_after_configure:
            if coverage:
                return self._coverage_compile_only(
                    project=project,
                    build_dir=build_dir,
                    lcov_file=build_dir / "lcov.info",
                    is_configured=True,
                )
            else:
                return self.build(build_dir=build_dir)

    def build(self, build_dir, output_files_out=None, fail_on_warnings=False):
        """Build a pre-configured build directory."""

        def wait_and_check_success(procs, writers):
            """Wait for processes to complete and check for errors

            Args:
                procs: List of subprocess.Popen objects to check
                writers: List of LogWriter objects to check

            Returns:
                True if all if OK
                False if an error was found (so that zmake should exit)
            """
            bad = None
            for proc in procs:
                if proc.wait() and not bad:
                    bad = proc
            if bad:
                # Just show the first bad process for now. Both builds likely
                # produce the same error anyway. If they don't, the user can
                # still take action on the errors/warnings provided. Showing
                # multiple 'Execution failed' messages is not very friendly
                # since it exposes the fragmented nature of the build.
                raise OSError(get_process_failure_msg(bad))

            # Let all output be produced before exiting
            for writer in writers:
                writer.wait()
            if fail_on_warnings and any(
                w.has_written(logging.WARNING) or w.has_written(logging.ERROR)
                for w in writers
            ):
                self.logger.warning("zmake: Warnings detected in build: aborting")
                return False
            return True

        procs = []
        log_writers = []
        dirs = {}

        build_dir = build_dir.resolve()
        found_projects = zmake.project.find_projects(build_dir / "project")
        project = found_projects[(build_dir / "project_name.txt").read_text()]

        # Compute the version string.
        version_string = zmake.version.get_version_string(
            project,
            build_dir / "zephyr_base",
            zmake.modules.locate_from_directory(build_dir / "modules"),
        )

        # The version header needs to generated during the build phase
        # instead of configure, as the tree may have changed since
        # configure was run.
        zmake.version.write_version_header(
            version_string,
            build_dir / "include" / "ec_version.h",
        )

        for build_name, build_config in project.iter_builds():
            with self.jobserver.get_job():
                dirs[build_name] = build_dir / "build-{}".format(build_name)
                cmd = ["/usr/bin/ninja", "-C", dirs[build_name].as_posix()]
                self.logger.info(
                    "Building %s:%s: %s",
                    build_dir,
                    build_name,
                    zmake.util.repr_command(cmd),
                )
                proc = self.jobserver.popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    encoding="utf-8",
                    errors="replace",
                )
                job_id = "{}:{}".format(build_dir, build_name)
                out = zmake.multiproc.log_output(
                    logger=self.logger,
                    log_level=logging.INFO,
                    file_descriptor=proc.stdout,
                    log_level_override_func=ninja_stdout_log_level_override,
                    job_id=job_id,
                )
                err = zmake.multiproc.log_output(
                    self.logger,
                    logging.ERROR,
                    proc.stderr,
                    job_id=job_id,
                )

                if self._sequential:
                    if not wait_and_check_success([proc], [out, err]):
                        return 2
                else:
                    procs.append(proc)
                    log_writers += [out, err]

        if not wait_and_check_success(procs, log_writers):
            return 2

        # Run the packer.
        packer_work_dir = build_dir / "packer"
        output_dir = build_dir / "output"
        for d in output_dir, packer_work_dir:
            if not d.exists():
                d.mkdir()

        if output_files_out is None:
            output_files_out = []
        for output_file, output_name in project.packer.pack_firmware(
            packer_work_dir, self.jobserver, version_string=version_string, **dirs
        ):
            shutil.copy2(output_file, output_dir / output_name)
            self.logger.debug("Output file '%s' created.", output_file)
            output_files_out.append(output_file)

        return 0

    def test(self, build_dir):
        """Test a build directory."""
        procs = []
        output_files = []
        self.build(build_dir, output_files_out=output_files)

        # If the project built but isn't a test, just bail.
        found_projects = zmake.project.find_projects(build_dir / "project")
        project = found_projects[(build_dir / "project_name.txt").read_text()]
        if not project.config.is_test:
            return 0

        for output_file in output_files:
            self.logger.info("Running tests in %s.", output_file)
            with self.jobserver.get_job():
                proc = self.jobserver.popen(
                    [output_file],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    encoding="utf-8",
                    errors="replace",
                )
                job_id = "test {}".format(output_file)
                zmake.multiproc.log_output(
                    self.logger,
                    logging.DEBUG,
                    proc.stdout,
                    job_id=job_id,
                )
                zmake.multiproc.log_output(
                    self.logger,
                    logging.ERROR,
                    proc.stderr,
                    job_id=job_id,
                )
                procs.append(proc)

        for idx, proc in enumerate(procs):
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))
        return 0

    def testall(self):
        """Test all the valid test targets"""
        tmp_dirs = []
        for project in zmake.project.find_projects(
            self.module_paths["ec"] / "zephyr"
        ).values():
            is_test = project.config.is_test
            temp_build_dir = tempfile.mkdtemp(
                suffix="-{}".format(project.config.project_name),
                prefix="zbuild-",
            )
            tmp_dirs.append(temp_build_dir)
            # Configure and run the test.
            self.executor.append(
                func=lambda: self._configure(
                    project=project,
                    build_dir=pathlib.Path(temp_build_dir),
                    build_after_configure=True,
                    test_after_configure=is_test,
                )
            )

        rv = self.executor.wait()
        for tmpdir in tmp_dirs:
            shutil.rmtree(tmpdir)
        return rv

    def _run_lcov(self, build_dir, lcov_file, initial=False, gcov=""):
        gcov = os.path.abspath(gcov)
        with self.jobserver.get_job():
            if initial:
                self.logger.info("Running (initial) lcov on %s.", build_dir)
            else:
                self.logger.info("Running lcov on %s.", build_dir)
            cmd = [
                "/usr/bin/lcov",
                "--gcov-tool",
                gcov,
                "-q",
                "-o",
                "-",
                "-c",
                "-d",
                build_dir,
                "-t",
                lcov_file.stem,
                "--rc",
                "lcov_branch_coverage=1",
                "--exclude",
                "*/build-*/zephyr/*/generated/*",
                "--exclude",
                "*/ec/test/*",
                "--exclude",
                "*/ec/zephyr/shim/chip/npcx/npcx_monitor/*",
                "--exclude",
                "*/ec/zephyr/emul/*",
                "--exclude",
                "*/ec/zephyr/test/*",
                "--exclude",
                "*/testsuite/*",
                "--exclude",
                "*/subsys/emul/*",
            ]
            if initial:
                cmd += ["-i"]
            proc = self.jobserver.popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            zmake.multiproc.log_output(
                self.logger,
                logging.WARNING,
                proc.stderr,
                job_id="{}-lcov".format(build_dir),
            )

            with open(lcov_file, "w") as outfile:
                for line in proc.stdout:
                    if line.startswith("SF:"):
                        path = line[3:].rstrip()
                        outfile.write("SF:%s\n" % os.path.realpath(path))
                    else:
                        outfile.write(line)
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))

            return 0

    def _coverage_compile_only(
        self, project, build_dir, lcov_file, is_configured=False
    ):
        self.logger.info("Building %s in %s", project.config.project_name, build_dir)
        if not is_configured:
            rv = self._configure(
                project=project,
                build_dir=build_dir,
                build_after_configure=False,
                test_after_configure=False,
                coverage=True,
            )
            if rv:
                return rv

        # Compute the version string.
        version_string = zmake.version.get_version_string(
            project,
            build_dir / "zephyr_base",
            zmake.modules.locate_from_directory(build_dir / "modules"),
        )

        # The version header needs to generated during the build phase
        # instead of configure, as the tree may have changed since
        # configure was run.
        zmake.version.write_version_header(
            version_string,
            build_dir / "include" / "ec_version.h",
        )

        # Use ninja to compile the all.libraries target.
        found_projects = zmake.project.find_projects(build_dir / "project")
        build_project = found_projects[(build_dir / "project_name.txt").read_text()]

        procs = []
        dirs = {}
        gcov = "gcov.sh-not-found"
        for build_name, build_config in build_project.iter_builds():
            self.logger.info("Building %s:%s all.libraries.", build_dir, build_name)
            dirs[build_name] = build_dir / "build-{}".format(build_name)
            gcov = dirs[build_name] / "gcov.sh"
            proc = self.jobserver.popen(
                ["/usr/bin/ninja", "-C", dirs[build_name], "all.libraries"],
                # Ninja will connect as a job client instead and claim
                # many jobs.
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            job_id = "{}:{}".format(build_dir, build_name)
            zmake.multiproc.log_output(
                logger=self.logger,
                log_level=logging.DEBUG,
                file_descriptor=proc.stdout,
                log_level_override_func=ninja_stdout_log_level_override,
                job_id=job_id,
            )
            zmake.multiproc.log_output(
                self.logger,
                logging.ERROR,
                proc.stderr,
                job_id=job_id,
            )
            if self._sequential:
                if proc.wait():
                    raise OSError(get_process_failure_msg(proc))
            else:
                procs.append(proc)

        for proc in procs:
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))

        return self._run_lcov(build_dir, lcov_file, initial=True, gcov=gcov)

    def _coverage_run_test(
        self,
        project,
        build_dir,
        lcov_file,
        is_configured=False,
    ):
        self.logger.info(
            "Running test %s in %s", project.config.project_name, build_dir
        )
        if not is_configured:
            rv = self._configure(
                project=project,
                build_dir=build_dir,
                build_after_configure=True,
                test_after_configure=True,
                coverage=True,
            )
            if rv:
                return rv
        gcov = "gcov.sh-not-found"
        for build_name, build_config in project.iter_builds():
            gcov = build_dir / "build-{}".format(build_name) / "gcov.sh"
        return self._run_lcov(build_dir, lcov_file, initial=False, gcov=gcov)

    def coverage(self, build_dir):
        """Builds all targets with coverage enabled, and then runs the tests."""
        all_lcov_files = []
        root_dir = self.module_paths["ec"] / "zephyr"
        for project in zmake.project.find_projects(root_dir).values():
            is_test = project.config.is_test
            project_build_dir = pathlib.Path(build_dir) / project.config.project_name
            lcov_file = pathlib.Path(build_dir) / "{}.info".format(
                project.config.project_name
            )
            if is_test:
                # Configure and run the test.
                all_lcov_files.append(lcov_file)
                self.executor.append(
                    func=lambda: self._coverage_run_test(
                        project, project_build_dir, lcov_file
                    )
                )
            else:
                # Don't build non-test projects
                self.logger.info("Skipping project %s", project.config.project_name)
            if self._sequential:
                rv = self.executor.wait()
                if rv:
                    return rv

        rv = self.executor.wait()
        if rv:
            return rv

        with self.jobserver.get_job():
            # Merge info files into a single lcov.info
            self.logger.info("Merging coverage data into %s.", build_dir / "lcov.info")
            cmd = [
                "/usr/bin/lcov",
                "-o",
                build_dir / "lcov.info",
                "--rc",
                "lcov_branch_coverage=1",
            ]
            for info in all_lcov_files:
                cmd += ["-a", info]
            proc = self.jobserver.popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            zmake.multiproc.log_output(
                self.logger, logging.ERROR, proc.stderr, job_id="lcov"
            )
            zmake.multiproc.log_output(
                self.logger, logging.DEBUG, proc.stdout, job_id="lcov"
            )
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))

            # Find the common root dir
            prefixdir = os.path.commonprefix(list(self.module_paths.values()))

            # Merge into a nice html report
            self.logger.info("Creating coverage report %s.", build_dir / "coverage_rpt")
            proc = self.jobserver.popen(
                [
                    "/usr/bin/genhtml",
                    "-q",
                    "-o",
                    build_dir / "coverage_rpt",
                    "-t",
                    "Zephyr EC Unittest",
                    "-p",
                    prefixdir,
                    "-s",
                    "--branch-coverage",
                ]
                + all_lcov_files,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            zmake.multiproc.log_output(
                self.logger, logging.ERROR, proc.stderr, job_id="genhtml"
            )
            zmake.multiproc.log_output(
                self.logger, logging.DEBUG, proc.stdout, job_id="genhtml"
            )
            if proc.wait():
                raise OSError(get_process_failure_msg(proc))
            return 0

    def list_projects(self, format, search_dir):
        """List project names known to zmake on stdout.

        Args:
            format: The formatting string to print projects with.
            search_dir: Directory to start the search for
                BUILD.py files at.
        """
        if not search_dir:
            search_dir = self.module_paths["ec"] / "zephyr"

        for project in zmake.project.find_projects(search_dir).values():
            print(format.format(config=project.config), end="")

        return 0

    def generate_readme(self, output_file, diff=False):
        """Re-generate the auto-generated README file.

        Args:
            output_file: A pathlib.Path; to be written only if changed.
            diff: Instead of writing out, report the diff.
        """
        expected_contents = zmake.generate_readme.generate_readme()

        if output_file.is_file():
            current_contents = output_file.read_text()
            if expected_contents == current_contents:
                return 0
            if diff:
                self.logger.error(
                    "The auto-generated README.md differs from the expected contents:"
                )
                for line in difflib.unified_diff(
                    current_contents.splitlines(keepends=True),
                    expected_contents.splitlines(keepends=True),
                    str(output_file),
                ):
                    self.logger.error(line.rstrip())
                self.logger.error('Run "zmake generate-readme" to fix this.')
                return 1

        if diff:
            self.logger.error(
                'The README.md file does not exist.  Run "zmake generate-readme".'
            )
            return 1

        output_file.write_text(expected_contents)
        return 0
