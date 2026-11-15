"""Keep author/home paths out of executable assertion and diagnostic strings."""
from pathlib import Path

Import("env")

project = Path(env.subst("$PROJECT_DIR")).resolve()
source = Path(env.subst("$PROJECT_SRC_DIR")).resolve()
mapping = [(Path.home().resolve(), "BUILD_HOME"), (project, "PROJECT"), (source, "firmware/src")]
for package, label in [("framework-arduinoespressif32", "arduino-framework"),
                       ("toolchain-xtensa-esp32s3", "xtensa-toolchain")]:
    folder = env.PioPlatform().get_package_dir(package)
    if folder:
        mapping.append((Path(folder).resolve(), label))
extra = env.GetProjectOption("lib_extra_dirs", [])
if isinstance(extra, str):
    extra = extra.splitlines()
for index, folder in enumerate(extra):
    folder = Path(folder.strip())
    if str(folder) and str(folder) != '.':
        mapping.append(((project / folder).resolve(), "third_party" + str(index)))
# More specific mappings follow their parent directories.
flags = []
for folder, label in sorted(mapping, key=lambda item: len(str(item[0]))):
    flags += ["-ffile-prefix-map=" + str(folder) + "=" + label,
              "-fmacro-prefix-map=" + str(folder) + "=" + label]
env.Append(CCFLAGS=flags)
