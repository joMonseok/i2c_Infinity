import shutil
from pathlib import Path

# 배포할 파일들 (헤더 + 구현 파일 세트)
files_to_sync = [
    "I2CProtocol.h",
    "I2CProtocol.cpp"  # 또는 headerTest.cpp
]

# 대상 프로젝트들
projects = ["plant", "sensor_Right", "sensor_LED"]

script_dir = Path(__file__).parent

for project in projects:
    project_dir = script_dir / project
    if project_dir.exists():
        for filename in files_to_sync:
            source = script_dir / filename
            target = project_dir / filename
            if source.exists():
                shutil.copy2(source, target)
                print(f"✓ Copied {filename} to {project}")
            else:
                print(f"⚠ {filename} not found")
    else:
        print(f"⚠ Project folder {project} not found")

print("\n배포 완료!")