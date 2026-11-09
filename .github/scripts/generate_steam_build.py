"""Generate a SteamPipe app build with Warfork's explicit depot IDs."""

import argparse
import pathlib
import re


DEPOTS = {
    671610: {671615: "windows", 671617: "linux", 671618: "macos"},
    1136510: {1136514: "windows", 1136517: "macos", 1136518: "linux"},
}


def quoted(value: str) -> str:
    if any(char in value for char in ('"', "\\", "\n", "\r")):
        raise ValueError(f"SteamPipe value contains an unsupported character: {value!r}")
    return f'"{value}"'


def generate(app_id: int, branch: str, description: str, content_root: pathlib.Path,
             build_output: pathlib.Path) -> str:
    if app_id not in DEPOTS:
        raise ValueError(f"Unknown Steam app ID: {app_id}")
    if branch == "default" or not re.fullmatch(r"[A-Za-z0-9_-]+", branch):
        raise ValueError(f"Invalid Steam beta branch: {branch!r}")
    if not content_root.is_dir():
        raise ValueError(f"Missing Steam content root: {content_root}")

    lines = [
        '"AppBuild"', "{",
        f'  "AppID" {quoted(str(app_id))}',
        f'  "Desc" {quoted(description)}',
        f'  "ContentRoot" {quoted(str(content_root.resolve()))}',
        f'  "BuildOutput" {quoted(str(build_output.resolve()))}',
        f'  "SetLive" {quoted(branch)}',
        '  "Depots"', "  {",
    ]
    for depot_id, folder in DEPOTS[app_id].items():
        directory = content_root / folder
        if not directory.is_dir() or not any(path.is_file() for path in directory.rglob("*")):
            raise ValueError(f"Missing or empty depot folder: {directory}")
        lines += [
            f'    {quoted(str(depot_id))}', "    {",
            '      "FileMapping"', "      {",
            f'        "LocalPath" {quoted(folder + "/*")}',
            '        "DepotPath" "."',
            '        "Recursive" "1"',
            "      }", "    }",
        ]
    lines += ["  }", "}"]
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-id", type=int, required=True)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--description", required=True)
    parser.add_argument("--content-root", type=pathlib.Path, required=True)
    parser.add_argument("--build-output", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    script = generate(args.app_id, args.branch, args.description,
                      args.content_root, args.build_output)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(script)


if __name__ == "__main__":
    main()
