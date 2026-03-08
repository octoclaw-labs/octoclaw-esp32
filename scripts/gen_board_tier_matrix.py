#!/usr/bin/env python3
"""
根据 Kconfig.projbuild 里的 BOARD_TYPE 选择项生成：
1) main/extensions/octoclaw/profile/board_tier_matrix.generated.h
2) docs/octoclaw-board-tier-matrix.md

支持 --check 模式做一致性校验。
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass
class BoardEntry:
    symbol: str
    depends_expr: str
    chip: str
    tier: str
    feature_expr: str


ROOT = Path(__file__).resolve().parents[1]
KCONFIG_PATH = ROOT / "main" / "Kconfig.projbuild"
HEADER_PATH = (
    ROOT
    / "main"
    / "extensions"
    / "octoclaw"
    / "profile"
    / "board_tier_matrix.generated.h"
)
DOC_PATH = ROOT / "docs" / "octoclaw-board-tier-matrix.md"


def normalize_space(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def parse_board_entries(kconfig_text: str) -> list[BoardEntry]:
    lines = kconfig_text.splitlines()
    start = None
    end = None

    for idx, line in enumerate(lines):
        if line.strip() == "choice BOARD_TYPE":
            start = idx
            break
    if start is None:
        raise RuntimeError("未找到 choice BOARD_TYPE")

    for idx in range(start + 1, len(lines)):
        if lines[idx].strip() == "endchoice":
            end = idx
            break
    if end is None:
        raise RuntimeError("未找到 BOARD_TYPE 的 endchoice")

    block = lines[start:end]
    entries: list[BoardEntry] = []

    i = 0
    while i < len(block):
        line = block[i].strip()
        m = re.match(r"config\s+(BOARD_TYPE_[A-Za-z0-9_]+)", line)
        if not m:
            i += 1
            continue

        symbol = m.group(1)
        depends = ""
        j = i + 1
        while j < len(block):
            stripped = block[j].strip()
            if stripped.startswith("config ") or stripped == "endchoice":
                break
            if stripped.startswith("depends on "):
                expr = stripped[len("depends on ") :].strip()
                while expr.endswith("\\") and j + 1 < len(block):
                    expr = expr[:-1] + " " + block[j + 1].strip()
                    j += 1
                depends = normalize_space(expr)
                break
            j += 1

        chip = detect_chip(depends)
        tier, feature_expr = to_tier_and_features(chip)
        entries.append(
            BoardEntry(
                symbol=symbol,
                depends_expr=depends if depends else "UNKNOWN",
                chip=chip,
                tier=tier,
                feature_expr=feature_expr,
            )
        )
        i = j

    return entries


def detect_chip(depends_expr: str) -> str:
    expr = depends_expr.upper()
    if "IDF_TARGET_ESP32P4" in expr:
        return "ESP32P4"
    if "IDF_TARGET_ESP32S3" in expr:
        return "ESP32S3"
    if "IDF_TARGET_ESP32C6" in expr:
        return "ESP32C6"
    if "IDF_TARGET_ESP32C5" in expr:
        return "ESP32C5"
    if "IDF_TARGET_ESP32C3" in expr:
        return "ESP32C3"
    if "IDF_TARGET_ESP32" in expr:
        return "ESP32"
    return "UNKNOWN"


def to_tier_and_features(chip: str) -> tuple[str, str]:
    if chip == "ESP32P4":
        return (
            "pro",
            "kFeaturePolicyGuard | kFeatureReceiptCompensation | "
            "kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent",
        )
    if chip == "ESP32S3":
        return (
            "standard",
            "kFeaturePolicyGuard | kFeatureReceiptCompensation | "
            "kFeatureLocalAgent | kFeatureMcpExtFields",
        )
    return (
        "lite",
        "kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields",
    )


def render_header(entries: Iterable[BoardEntry]) -> str:
    entries = list(entries)
    lines = [
        "#ifndef OCTO_BOARD_TIER_MATRIX_GENERATED_H",
        "#define OCTO_BOARD_TIER_MATRIX_GENERATED_H",
        "",
        "// 此文件由 scripts/gen_board_tier_matrix.py 自动生成，请勿手工修改。",
        "",
        "namespace octo {",
        "",
        "static constexpr BoardProfile kGeneratedBoardProfiles[] = {",
    ]

    tier_enum = {
        "lite": "BoardTier::kLite",
        "standard": "BoardTier::kStandard",
        "pro": "BoardTier::kPro",
    }

    for entry in entries:
        lines.append(
            f'    {{"{entry.symbol}", "{entry.chip}", {tier_enum[entry.tier]}, {entry.feature_expr}}},'
        )
    lines.append("};")
    lines.append("")
    lines.append(
        f"static constexpr size_t kGeneratedBoardProfileCount = {len(entries)};"
    )
    lines.append("")
    lines.append("inline const BoardProfile& GetCompiledBoardProfileGenerated() {")

    first = True
    for idx, entry in enumerate(entries):
        prefix = "#if" if first else "#elif"
        lines.append(f"{prefix} defined(CONFIG_{entry.symbol})")
        lines.append(f"    return kGeneratedBoardProfiles[{idx}];")
        first = False

    lines.extend(
        [
            "#else",
            "    return kGeneratedBoardProfiles[0];",
            "#endif",
            "}",
            "",
            "}  // namespace octo",
            "",
            "#endif  // OCTO_BOARD_TIER_MATRIX_GENERATED_H",
            "",
        ]
    )
    return "\n".join(lines)


def render_doc(entries: Iterable[BoardEntry]) -> str:
    entries = list(entries)
    chip_count: dict[str, int] = {}
    for item in entries:
        chip_count[item.chip] = chip_count.get(item.chip, 0) + 1

    lines = [
        "# OctoClaw 全板型档位矩阵（自动生成）",
        "",
        "> 本文件由 `scripts/gen_board_tier_matrix.py` 生成，请勿手工编辑。",
        "",
        f"- 板型总数：**{len(entries)}**",
        "- 档位规则：Lite=ESP32/C3/C5/C6，Standard=ESP32S3，Pro=ESP32P4",
        "",
        "## 芯片分布统计",
        "",
        "| 芯片 | 数量 |",
        "|---|---:|",
    ]
    for chip in sorted(chip_count.keys()):
        lines.append(f"| {chip} | {chip_count[chip]} |")

    lines.extend(
        [
            "",
            "## 全板型清单",
            "",
            "| BOARD_TYPE 符号 | 芯片 | 档位 | 默认 FeatureMask |",
            "|---|---|---|---|",
        ]
    )

    for entry in entries:
        lines.append(
            f"| `{entry.symbol}` | {entry.chip} | {entry.tier} | `{entry.feature_expr}` |"
        )

    lines.append("")
    return "\n".join(lines)


def write_or_check(path: Path, content: str, check: bool) -> bool:
    if check:
        if not path.exists():
            print(f"[CHECK] 缺少文件: {path}")
            return False
        current = path.read_text(encoding="utf-8")
        if current != content:
            print(f"[CHECK] 文件内容不一致: {path}")
            return False
        return True

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="生成 OctoClaw 板型档位矩阵")
    parser.add_argument("--check", action="store_true", help="仅校验，不写文件")
    args = parser.parse_args()

    text = KCONFIG_PATH.read_text(encoding="utf-8")
    entries = parse_board_entries(text)
    if len(entries) != 131:
        print(f"[ERROR] BOARD_TYPE 数量异常，期望 131，实际 {len(entries)}")
        return 2

    header = render_header(entries)
    doc = render_doc(entries)

    ok = True
    ok &= write_or_check(HEADER_PATH, header, args.check)
    ok &= write_or_check(DOC_PATH, doc, args.check)
    if not ok:
        return 1

    mode = "CHECK" if args.check else "WRITE"
    print(f"[{mode}] board matrix ok, entries={len(entries)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
