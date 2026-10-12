"""Run host tests for the MShell parser and input/output path."""

from pathlib import Path
import os
import shutil
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
FALLBACK_CC = Path("D:/0_software/msys64/mingw64/bin/gcc.exe")
FALLBACK_ARM_CC = Path("D:/0_software/llvm_for_arm/bin/clang.exe")
RUN_ENV = None


def run(args, success=True):
    result = subprocess.run(list(map(str, args)), capture_output=True,
                           text=True, env=RUN_ENV)
    if (result.returncode == 0) != success:
        raise AssertionError(
            f"Command: {args}\n{result.stdout}\n{result.stderr}")
    return result.stdout + result.stderr


def main():
    global RUN_ENV
    compiler = os.environ.get("HOST_CC") or shutil.which("gcc")
    if compiler is None and FALLBACK_CC.is_file():
        compiler = str(FALLBACK_CC)
    if compiler is None:
        raise RuntimeError("Set HOST_CC to a C11 host compiler")
    arm = os.environ.get("ARM_CC") or shutil.which("clang")
    if arm is None and FALLBACK_ARM_CC.is_file():
        arm = str(FALLBACK_ARM_CC)
    if arm is None:
        raise RuntimeError("Set ARM_CC to the clang cross-compiler")
    cxx = os.environ.get("HOST_CXX") or shutil.which("g++")
    fallback_cxx = Path(compiler).with_name("g++.exe")
    if cxx is None and fallback_cxx.is_file():
        cxx = str(fallback_cxx)
    if cxx is None:
        raise RuntimeError("Set HOST_CXX to a C++ compiler")
    RUN_ENV = os.environ.copy()
    RUN_ENV["PATH"] = os.pathsep.join(
        [str(Path(compiler).resolve().parent),
         str(Path(arm).resolve().parent),
         str(Path(cxx).resolve().parent), RUN_ENV.get("PATH", "")])

    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror",
             f"-I{HERE / 'stubs'}", f"-I{ROOT / 'src'}",
             f"-I{ROOT / 'src/mdebug'}"]
    with tempfile.TemporaryDirectory(prefix="mshell_") as temp:
        work = Path(temp)
        parser = work / "test_mshell_parse.exe"
        run([compiler, *flags, HERE / "test_mshell_parse.c",
             ROOT / "src/mdebug/mshell_parse.c", "-o", parser])
        run([parser])
        print("PASS parser token, integer, float, bool, hex and generic contracts")

        io = work / "test_mshell_io.exe"
        run([compiler, *flags, "-DMSHELL_USE_RTT_DEFAULT=0",
             HERE / "test_mshell_io.c",
             ROOT / "src/mdebug/mshell.c",
             ROOT / "src/mdebug/mshell_parse.c", "-o", io])
        run([io])
        print("PASS fake IO, Tab, backspace, overflow recovery and replies")

        for target, source, include in (
            ("--target=armv7em-none-eabi", "src/arch/cortex-m/mdebug_cm.c",
             "src/arch/cortex-m"),
            ("--target=riscv32-unknown-elf", "src/arch/riscv/mdebug_riscv.c",
             "src/arch/riscv"),
        ):
            command = [arm, target, "-std=c11", "-Wall", "-Wextra",
                       "-Werror", "-fsyntax-only", f"-I{HERE / 'stubs'}",
                       f"-I{ROOT / 'src'}", f"-I{ROOT / include}"]
            if "armv7em" in target:
                command.extend(["-mcpu=cortex-m4", "-mthumb"])
            else:
                command.extend(["-march=rv32imac", "-mabi=ilp32"])
            run([*command, ROOT / source])
            print(f"PASS syntax check {source}")

        unsupported = work / "unsupported.c"
        unsupported.write_text(
            '#include "mdebug/mshell_parse.h"\n'
            'void bad(void) { const char *p = "1"; double x = 0.0; '
            '(void)mshell_Parse(&p, &x); }\n', encoding="utf-8")
        output = run([compiler, *flags, "-fsyntax-only", unsupported], False)
        assert "mshell_Parse" in output
        print("PASS rejects unsupported generic destination type")

        explicit = work / "explicit.c"
        explicit.write_text(
            '#include "mdebug/mshell_parse.h"\n'
            'int f(void) { const char *p = "0.05"; float v = 0.0f; '
            'return (int)mshell_ParseFloat(&p, &v); }\n',
            encoding="utf-8")
        run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
             *flags[3:], "-fsyntax-only", explicit])
        print("PASS explicit typed API remains available in C99")

        parser_object = work / "mshell_parse.o"
        run([compiler, *flags, "-c", ROOT / "src/mdebug/mshell_parse.c",
             "-o", parser_object])
        cpp = work / "cpp_api.cpp"
        cpp.write_text(
            '#include "mdebug/mshell_parse.h"\n'
            'int main() { const char *p = "0.05"; float v = 0.0f; '
            'return mshell_ParseFloat(&p, &v) == MSHELL_PARSE_OK '
            '&& v == 0.05f ? 0 : 1; }\n',
            encoding="utf-8")
        cpp_exe = work / "cpp_api.exe"
        run([cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror",
             f"-I{ROOT / 'src'}", cpp, parser_object, "-o", cpp_exe])
        run([cpp_exe])
        print("PASS explicit typed API C++ linkage")


if __name__ == "__main__":
    main()
