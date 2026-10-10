"""Run host contracts, compile-negative cases and Cortex-M4 codegen probes.

Usage from the template root: python modus/src/mdi/tests/run_tests.py
Override tool locations with HOST_CC, ARM_CC, ARM_OBJDUMP if necessary.
All compilation products live in one temporary directory.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
from collections import Counter

HERE = Path(__file__).resolve().parent
MDI = HERE.parent
ROOT = HERE.parents[3]


def tool(variable, name, fallback):
    result = os.environ.get(variable) or shutil.which(name)
    if result:
        return result
    if Path(fallback).is_file():
        return fallback
    raise RuntimeError(f"Set {variable} to the required {name} executable")


def run(args, success=True):
    result = subprocess.run(list(map(str, args)), capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(f"Command: {args}\n{result.stdout}\n{result.stderr}")
    return result.stdout + result.stderr


def instructions(disassembly):
    bodies = {}
    current = None
    for line in disassembly.splitlines():
        match = re.match(r"^[0-9a-f]+ <(\w+)>:", line)
        if match:
            current = match.group(1)
            bodies[current] = []
        elif current:
            match = re.match(r"\s*[0-9a-f]+:\s+(.+)", line)
            if match:
                instruction = " ".join(match.group(1).split())
                instruction = re.sub(r"<\w+([+-][^>]*)?>", r"<local\1>",
                                     instruction)
                bodies[current].append(instruction)
    return bodies


def main():
    host = tool("HOST_CC", "gcc", "D:/0_software/msys64/mingw64/bin/gcc.exe")
    arm = tool("ARM_CC", "clang", "D:/0_software/llvm_for_arm/bin/clang.exe")
    dump = tool("ARM_OBJDUMP", "llvm-objdump",
                "D:/0_software/llvm_for_arm/bin/llvm-objdump.exe")
    os.environ["PATH"] = os.pathsep.join(
        [str(Path(exe).resolve().parent) for exe in (host, arm, dump)]
        + [os.environ.get("PATH", "")])
    common = ["-std=c11", "-Wall", "-Wextra", "-Werror",
              "-Wno-int-to-pointer-cast",
              f"-I{MDI}", f"-I{HERE}", f"-I{ROOT / 'modus/src'}",
              f"-I{ROOT / 'modus/src/utilities'}",
              f"-I{ROOT / 'peripheral/stm32g431'}",
              f"-I{ROOT / 'vendor/cortex-m/cmsis_device_g4/Include'}",
              f"-I{ROOT / 'vendor/cortex-m/cmsis_core'}"]
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="mdi_", dir=build) as temp:
        work = Path(temp)
        public = work / "public_header.c"
        public.write_text('#include "mdi/mdi.h"\n', encoding="utf-8")
        public_flags = ["-std=c11", "-Wall", "-Wextra", "-Werror",
                        f"-I{ROOT / 'modus/src'}"]
        run([host, *public_flags, "-fsyntax-only", public])
        dependencies = run([host, *public_flags, "-M", public])
        assert "mdi/feature/" not in dependencies.replace("\\", "/")
        print("PASS standalone public header: core only")

        exe = work / ("contract.exe" if os.name == "nt" else "contract")
        run([host, *common, HERE / "contract.c", "-o", exe])
        run([exe])
        print("PASS host IO/masked-IO/sample/PWM/frequency/soft-I2C/SPI-EEPROM contracts")

        exe = work / ("g431_pwm.exe" if os.name == "nt" else "g431_pwm")
        run([host, *common,
             ROOT / "peripheral/stm32g431/mdi/tests/pwm_runtime.c", "-o", exe])
        run([exe])
        print("PASS G431 PWM lifecycle and fault register contracts")

        template = ROOT / "peripheral/template"
        perfc_flags = ["-std=gnu11",
                          f"-I{ROOT / 'modus/lib/perf_counter'}",
                          f"-I{ROOT / 'modus/src/arch'}",
                          "-D__PERFC_USE_USER_CUSTOM_PORTING__=1",
                          '-D__PERFC_CFG_PORTING_INCLUDE__="perfc_port.h"',
                          "-D__COMPILER_HAS_GNU_EXTENSIONS__=1"]
        template_flags = [f"-I{template}", *common, *perfc_flags]
        run([host, *template_flags, "-fsyntax-only",
             template / "tests/contract.c"])
        for name in ("runtime", "service_runtime"):
            exe = work / (name + (".exe" if os.name == "nt" else ""))
            run([host, *template_flags, template / f"tests/{name}.c",
                 ROOT / "modus/src/utilities/mringbuf.c", "-o", exe])
            run([exe])
            print(f"PASS template {name}")

        exe = work / ("g431_adc.exe" if os.name == "nt" else "g431_adc")
        run([host, *common, *perfc_flags,
             ROOT / "peripheral/stm32g431/mdi/tests/adc_runtime.c",
             ROOT / "modus/src/utilities/mringbuf.c", "-o", exe])
        run([exe])
        print("PASS G431 regular DMA busy, publication and transfer error")

        source = (ROOT / "modus/src/modus.c").read_text(encoding="utf-8")
        start = source.index("void modus_Init(")
        end = source.index("\n/**", start)
        (work / "modus_init_under_test.h").write_text(source[start:end],
                                                     encoding="utf-8")
        exe = work / ("startup.exe" if os.name == "nt" else "startup")
        run([host, *common, f"-I{work}", HERE / "startup.c", "-o", exe])
        run([exe])
        print("PASS actual modus_Init: board ready before logs/business Init")

        negatives = {
            "wrong_sample_type": 'MDI_PWM_Frame(bridge) x = {0}; '
                                 'MDI_Sample_Read(current, &x);',
            "wrong_pwm_type": 'MDI_PWM_Frame(buzzer) x = {0}; '
                              '(void)MDI_PWM_Stage(bridge, &x);',
            "wrong_duty_units": 'MDI_PWM_Frame(variable) x = {0}; '
                                '(void)MDI_PWM_SetDuty(variable, &x);',
            "unsupported_capability": '(void)MDI_PWM_SetFrequency(led, 100U);',
            "unsupported_mean_frequency":
                '(void)MDI_ADC_SetSampleFrequency(test_adc_race_mean, 100U);',
            "input_only_write": '(void)MDI_IO_Write(input_only, 1U);',
            "output_only_read": '(void)MDI_IO_Read(output_only);',
        }
        for name, body in negatives.items():
            source = work / f"{name}.c"
            source.write_text('#include "fixtures.h"\nvoid misuse(void) {'
                              + body + '}\n', encoding="utf-8")
            output = run([host, *common, "-fsyntax-only", source], False)
            expected = "implicit declaration" if name.startswith((
                "unsupported", "input", "output")) \
                else "incompatible pointer"
            assert expected in output.lower(), output
            print(f"PASS rejects {name}")

        source = work / "g431_commit.c"
        source.write_text('#include "mdi/instance.h"\n'
                          'void misuse(void) {(void)MDI_PWM_Commit(bridge);}\n',
                          encoding="utf-8")
        output = run([host, *common, "-fsyntax-only", source], False)
        assert "implicit declaration" in output.lower(), output
        print("PASS rejects G431 bridge explicit Commit")

        maps = {
            "duplicate_logical": "X(V,0,1,0) X(V,0,2,0)",
            "duplicate_physical": "X(V,0,1,0) X(V,1,1,0)",
            "invalid_logical": "X(V,0,1,0) X(V,2,2,0)",
            "invalid_physical": "X(V,0,1,0) X(V,1,16,0)",
        }
        for name, pins in maps.items():
            source = work / f"{name}.c"
            source.write_text(
                '#include "fixtures.h"\n#define BAD_PINS(X,V) ' + pins
                + '\n#define BAD_PORTS(X,V) '
                'X(V,test_a_in,test_a_out,BAD_PINS)\n'
                'MDI_STM32_IO_BIND(bad,2,BAD_PORTS)\n', encoding="utf-8")
            output = run([host, *common, "-fsyntax-only", source], False)
            assert "static assertion failed" in output.lower(), output
            print(f"PASS rejects {name}")

        direction_negatives = {
            "input_caps_output":
                '#define P(X,V,...) X(V,0,4,0)\n'
                '#define Q(X,V) X(V,test_a_in,test_a_out,P)\n'
                'MDI_STM32_IO_BIND_INPUT_CAPS(bad,1,Q,MDI_IO_CAP_OUTPUT)',
            "output_caps_input":
                '#define P(X,V,...) X(V,0,4,0)\n'
                '#define Q(X,V) X(V,test_a_in,test_a_out,P)\n'
                'MDI_STM32_IO_BIND_OUTPUT_CAPS(bad,1,Q,MDI_IO_CAP_INPUT)',
        }
        for name, body in direction_negatives.items():
            source = work / f"{name}.c"
            source.write_text('#include "fixtures.h"\n' + body + '\n',
                              encoding="utf-8")
            output = run([host, *common, "-fsyntax-only", source], False)
            assert "static assertion failed" in output.lower(), output
            print(f"PASS rejects {name}")

        arm_flags = ["--target=armv7em-none-eabi", "-mcpu=cortex-m4", "-mthumb",
                     "-ffreestanding", "-fno-lto", "-ffunction-sections",
                     f"-I{ROOT / 'vendor/cortex-m/cmsis_core'}",
                     f"-I{ROOT / 'vendor/cortex-m/cmsis_device_g4/Include'}"]
        for opt in ("-O2", "-Os"):
            bodies = {}
            for mode, flags in (("g431", []),
                                ("fixtures", ["-DMDI_CODEGEN_FIXTURES=1"])):
                obj = work / f"probe_{mode}{opt}.o"
                run([arm, *common, *arm_flags, *flags, opt, "-c",
                     HERE / "codegen.c", "-o", obj])
                asm = run([dump, "-d", "--no-show-raw-insn", obj])
                bodies.update(instructions(asm))
            for name in ("led", "input", "bus", "bus_masked", "sample", "pwm", "edge"):
                direct, abstract = bodies[f"raw_{name}"], bodies[f"mdi_{name}"]
                assert direct and abstract
                if name == "bus":
                    assert Counter(x.split()[0] for x in direct) == \
                        Counter(x.split()[0] for x in abstract), \
                        f"{opt}/{name} instruction mix differs"
                    match = "same instruction mix; scheduling may differ"
                elif name == "bus_masked":
                    direct_stores = sum(x.split()[0].startswith("str")
                                        for x in direct)
                    abstract_stores = sum(x.split()[0].startswith("str")
                                          for x in abstract)
                    assert direct_stores == abstract_stores == 2, \
                        f"{opt}/{name} must issue one store per port"
                    match = "two direct port stores; scheduling may differ"
                else:
                    assert direct == abstract, \
                        f"{opt}/{name} differs:\n{direct}\n{abstract}"
                    match = "identical instructions"
                assert not any(re.match(r"bl(x)?\s", x) for x in abstract)
                print(f"PASS {opt} {name}: {len(direct)}, {match}")
            for name in ("adc_start", "safe_stop", "fault_active", "fault_clear"):
                body = bodies[f"mdi_{name}"]
                assert body and not any(re.match(r"bl(x)?\s", x) for x in body), \
                    f"{opt}/{name} contains a forwarding call: {body}"
                print(f"PASS {opt} G431 {name}: no forwarding call")
            run([arm, *common, *arm_flags, opt, "-fsyntax-only",
                 MDI / "examples/application.c"])
        print("PASS Cortex-M4 application example, no LTO")


if __name__ == "__main__":
    main()
