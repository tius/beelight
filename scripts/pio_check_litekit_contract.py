Import("env")

from pathlib import Path
import subprocess


# run the architecture contract check before each platformio build
project_dir = Path(env.subst("$PROJECT_DIR"))
python_exe = env.subst("$PYTHONEXE")
check_script = project_dir / "scripts" / "check_litekit_contract.py"

result = subprocess.run(
    [python_exe, str(check_script), "--root", str(project_dir)],
    cwd=str(project_dir),
    check=False,
)

if result.returncode != 0:
    env.Exit(result.returncode)
