#!/usr/bin/env python3
"""
将 research/openclaw/extensions 的 openclaw.plugin.json
翻译为 ESP32 侧可落地的扩展产物（目录、清单、矩阵、catalog 头文件）。
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass
class ExtensionEntry:
    id: str
    name: str
    source_dir: str
    adapter_type: str
    channels: list[str]
    providers: list[str]
    commands: list[str]
    skills: list[str]
    kind: str
    has_config_schema: bool
    config_key_count: int
    has_ui_hints: bool
    websocket_capable: bool
    websocket_evidence: list[str]
    status: str
    notes: str


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT.parent / "research" / "openclaw" / "extensions"
CHANNELS_ROOT = ROOT / "main" / "extensions" / "channels"
DOC_MATRIX_PATH = ROOT / "docs" / "openclaw-extension-translation-matrix.md"
INDEX_JSON_PATH = CHANNELS_ROOT / "openclaw_extensions_manifest.json"
GENERATED_HEADER_PATH = CHANNELS_ROOT / "openclaw_extension_catalog.generated.h"
OVERRIDES_PATH = CHANNELS_ROOT / "translation_status_overrides.json"

WEBSOCKET_HINT_PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    ("websocket", re.compile(r"websocket", re.IGNORECASE)),
    ("ws://", re.compile(r"ws://", re.IGNORECASE)),
    ("wss://", re.compile(r"wss://", re.IGNORECASE)),
    ("socket_mode", re.compile(r"socket[_\\s-]?mode", re.IGNORECASE)),
    ("socket.io", re.compile(r"socket\\.io", re.IGNORECASE)),
    ("eventsub", re.compile(r"eventsub", re.IGNORECASE)),
]


def detect_adapter_type(plugin: dict[str, Any]) -> str:
    if plugin.get("channels"):
        return "channel"
    if plugin.get("providers"):
        return "provider"
    if plugin.get("kind") == "memory":
        return "memory"
    if plugin.get("skills"):
        return "skill"
    if plugin.get("commands"):
        return "command"
    return "service"


def count_config_keys(plugin: dict[str, Any]) -> int:
    schema = plugin.get("configSchema")
    if not isinstance(schema, dict):
        return 0
    props = schema.get("properties")
    if not isinstance(props, dict):
        return 0
    return len(props)


def detect_websocket_evidence(extension_dir: Path) -> list[str]:
    evidence: list[str] = []
    for marker, pattern in WEBSOCKET_HINT_PATTERNS:
        found = False
        for path in extension_dir.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in {".ts", ".js", ".json", ".md", ".yaml", ".yml"}:
                continue
            content = path.read_text(encoding="utf-8", errors="ignore")
            if pattern.search(content):
                found = True
                break
        if found:
            evidence.append(marker)
    return evidence


def load_overrides() -> dict[str, dict[str, Any]]:
    if not OVERRIDES_PATH.exists():
        return {}
    raw = json.loads(OVERRIDES_PATH.read_text(encoding="utf-8"))
    if not isinstance(raw, dict):
        return {}
    return {str(k): v for k, v in raw.items() if isinstance(v, dict)}


def load_entries(source_root: Path, overrides: dict[str, dict[str, Any]]) -> list[ExtensionEntry]:
    entries: list[ExtensionEntry] = []
    for plugin_file in sorted(source_root.glob("*/openclaw.plugin.json")):
        plugin = json.loads(plugin_file.read_text(encoding="utf-8"))
        extension_dir = plugin_file.parent
        extension_id = plugin.get("id") or extension_dir.name
        adapter_type = detect_adapter_type(plugin)
        override = overrides.get(extension_id, {})
        status = str(override.get("status", "translated-skeleton"))
        notes = str(override.get("notes", ""))

        detected_evidence = detect_websocket_evidence(extension_dir)
        websocket_capable = adapter_type == "channel" and bool(detected_evidence)
        if "websocket_capable" in override:
            websocket_capable = bool(override.get("websocket_capable"))

        websocket_evidence: list[str] = []
        if adapter_type == "channel":
            override_evidence = override.get("websocket_evidence")
            if isinstance(override_evidence, list):
                websocket_evidence = [str(item) for item in override_evidence if str(item).strip()]
            else:
                websocket_evidence = detected_evidence
            if websocket_capable and not websocket_evidence:
                websocket_evidence = ["manual-override"]

        entries.append(
            ExtensionEntry(
                id=extension_id,
                name=extension_dir.name,
                source_dir=f"research/openclaw/extensions/{extension_dir.name}",
                adapter_type=adapter_type,
                channels=list(plugin.get("channels", [])),
                providers=list(plugin.get("providers", [])),
                commands=list(plugin.get("commands", [])),
                skills=list(plugin.get("skills", [])),
                kind=str(plugin.get("kind", "")),
                has_config_schema=isinstance(plugin.get("configSchema"), dict),
                config_key_count=count_config_keys(plugin),
                has_ui_hints=isinstance(plugin.get("uiHints"), dict),
                websocket_capable=websocket_capable,
                websocket_evidence=websocket_evidence,
                status=status,
                notes=notes,
            )
        )
    return entries


def build_todo_list(entry: ExtensionEntry) -> list[str]:
    if entry.status == "implemented":
        return ["已接入设备侧可运行实现并通过基础链路验证"]

    todos = [
        "补齐设备侧配置映射（NVS/menuconfig）",
        "补齐 MCP 工具与渠道事件的转换层",
        "补齐故障回执与遥测字段映射",
    ]
    if entry.adapter_type == "channel":
        if entry.websocket_capable:
            todos.append("补齐上行/下行消息路由与会话绑定（WebSocket 桥接）")
        else:
            todos.append("当前不纳入 ESP32 实现范围（仅实现可 WebSocket 直连的 channel）")
    elif entry.adapter_type == "provider":
        todos.append("补齐 provider 鉴权与配额保护")
    elif entry.adapter_type == "memory":
        todos.append("评估是否上移，设备侧仅保留轻量索引")
    elif entry.adapter_type == "skill":
        todos.append("拆分可设备化 skill，重能力上移云端")
    return todos


def render_extension_readme(entry: ExtensionEntry) -> str:
    lines = [
        f"# {entry.id}（ESP32 翻译状态）",
        "",
        f"- 源扩展：`{entry.source_dir}`",
        f"- 适配类型：`{entry.adapter_type}`",
        f"- 当前状态：`{entry.status}`",
    ]
    if entry.notes:
        lines.append(f"- 说明：{entry.notes}")

    lines.extend(
        [
            "",
            "## 能力摘要",
            "",
            f"- channels: `{', '.join(entry.channels) if entry.channels else '-'}`",
            f"- providers: `{', '.join(entry.providers) if entry.providers else '-'}`",
            f"- commands: `{', '.join(entry.commands) if entry.commands else '-'}`",
            f"- skills: `{', '.join(entry.skills) if entry.skills else '-'}`",
            f"- kind: `{entry.kind if entry.kind else '-'}`",
            f"- 配置键数量: `{entry.config_key_count}`",
        ]
    )
    if entry.adapter_type == "channel":
        lines.append(f"- WebSocket 直连可行: `{'yes' if entry.websocket_capable else 'no'}`")
        lines.append(
            f"- WebSocket 证据: `{', '.join(entry.websocket_evidence) if entry.websocket_evidence else '-'}`"
        )

    lines.extend(["", "## 设备侧待实现", ""])

    for todo in build_todo_list(entry):
        prefix = "[x]" if entry.status == "implemented" else "[ ]"
        lines.append(f"- {prefix} {todo}")

    lines.append("")
    return "\n".join(lines)


def render_manifest_json(entry: ExtensionEntry) -> dict[str, Any]:
    data = {
        "id": entry.id,
        "source": {
            "path": entry.source_dir,
            "pluginFile": f"{entry.source_dir}/openclaw.plugin.json",
        },
        "adapterType": entry.adapter_type,
        "status": entry.status,
        "translationVersion": 1,
        "capabilities": {
            "channels": entry.channels,
            "providers": entry.providers,
            "commands": entry.commands,
            "skills": entry.skills,
            "kind": entry.kind,
            "hasConfigSchema": entry.has_config_schema,
            "configKeyCount": entry.config_key_count,
            "hasUiHints": entry.has_ui_hints,
        },
        "esp32": {
            "transportHints": ["websocket", "mqtt", "nats"],
            "defaultEnabled": entry.status == "implemented",
            "websocketCapable": entry.websocket_capable,
            "websocketEvidence": entry.websocket_evidence,
            "implementationScope": (
                "in-scope"
                if entry.adapter_type != "channel" or entry.websocket_capable
                else "out-of-scope-websocket-only"
            ),
            "todos": build_todo_list(entry),
        },
    }
    if entry.notes:
        data["notes"] = entry.notes
    return data


def render_matrix(entries: list[ExtensionEntry]) -> str:
    type_count: dict[str, int] = {}
    status_count: dict[str, int] = {}
    channel_entries: list[ExtensionEntry] = []
    for entry in entries:
        type_count[entry.adapter_type] = type_count.get(entry.adapter_type, 0) + 1
        status_count[entry.status] = status_count.get(entry.status, 0) + 1
        if entry.adapter_type == "channel":
            channel_entries.append(entry)

    lines = [
        "# OpenClaw Extensions → ESP32 翻译矩阵（自动生成）",
        "",
        "> 由 `scripts/translate_openclaw_extensions.py` 生成，请勿手工修改。",
        "",
        f"- 扩展总数：**{len(entries)}**",
        "- 目标目录：`main/extensions/channels/<extension-id>`",
        "",
        "## 类型统计",
        "",
        "| 类型 | 数量 |",
        "|---|---:|",
    ]
    for adapter_type in sorted(type_count.keys()):
        lines.append(f"| `{adapter_type}` | {type_count[adapter_type]} |")

    lines.extend(
        [
            "",
            "## 状态统计",
            "",
            "| 状态 | 数量 |",
            "|---|---:|",
        ]
    )
    for status in sorted(status_count.keys()):
        lines.append(f"| `{status}` | {status_count[status]} |")

    websocket_channel_count = sum(1 for entry in channel_entries if entry.websocket_capable)
    lines.extend(
        [
            "",
            "## Channel WebSocket 适配范围",
            "",
            f"- channel 总数：**{len(channel_entries)}**",
            f"- 可 WebSocket 直连（纳入 ESP32 实现范围）：**{websocket_channel_count}**",
            f"- 暂不纳入（非 WebSocket 直连）：**{len(channel_entries) - websocket_channel_count}**",
        ]
    )

    lines.extend(
        [
            "",
            "## 全量清单",
            "",
            "| 扩展 ID | 适配类型 | channels | providers | 配置键 | WS-Channel | 状态 |",
            "|---|---|---:|---:|---:|---|---|",
        ]
    )

    for entry in entries:
        websocket_flag = "-"
        if entry.adapter_type == "channel":
            websocket_flag = "yes" if entry.websocket_capable else "no"
        lines.append(
            f"| `{entry.id}` | `{entry.adapter_type}` | {len(entry.channels)} | {len(entry.providers)} | {entry.config_key_count} | `{websocket_flag}` | `{entry.status}` |"
        )

    lines.append("")
    return "\n".join(lines)


def render_index_json(entries: list[ExtensionEntry]) -> dict[str, Any]:
    type_count: dict[str, int] = {}
    status_count: dict[str, int] = {}
    channel_entries: list[ExtensionEntry] = []
    for entry in entries:
        type_count[entry.adapter_type] = type_count.get(entry.adapter_type, 0) + 1
        status_count[entry.status] = status_count.get(entry.status, 0) + 1
        if entry.adapter_type == "channel":
            channel_entries.append(entry)

    websocket_channel_count = sum(1 for entry in channel_entries if entry.websocket_capable)

    return {
        "sourceRoot": "research/openclaw/extensions",
        "targetRoot": "main/extensions/channels",
        "total": len(entries),
        "typeCount": type_count,
        "statusCount": status_count,
        "channelScope": {
            "websocketOnlyPolicy": True,
            "channelTotal": len(channel_entries),
            "websocketCapable": websocket_channel_count,
            "outOfScope": len(channel_entries) - websocket_channel_count,
        },
        "extensions": [
            {
                "id": entry.id,
                "adapterType": entry.adapter_type,
                "sourceDir": entry.source_dir,
                "channels": entry.channels,
                "providers": entry.providers,
                "commands": entry.commands,
                "skills": entry.skills,
                "kind": entry.kind,
                "configKeyCount": entry.config_key_count,
                "websocketCapable": entry.websocket_capable,
                "websocketEvidence": entry.websocket_evidence,
                "status": entry.status,
                "notes": entry.notes,
            }
            for entry in entries
        ],
    }


def render_generated_header(entries: list[ExtensionEntry]) -> str:
    lines = [
        "#ifndef OCTO_OPENCLAW_EXTENSION_CATALOG_GENERATED_H",
        "#define OCTO_OPENCLAW_EXTENSION_CATALOG_GENERATED_H",
        "",
        "// 此文件由 scripts/translate_openclaw_extensions.py 自动生成，请勿手工修改。",
        "",
        "namespace octo::channels {",
        "",
        "static constexpr ExtensionCatalogEntry kOpenClawTranslatedExtensions[] = {",
    ]

    for entry in entries:
        lines.append(
            "    {"
            f"\"{entry.id}\", "
            f"\"{entry.adapter_type}\", "
            f"\"{entry.source_dir}\", "
            f"\"{entry.status}\", "
            f"{'true' if entry.channels else 'false'}, "
            f"{'true' if entry.providers else 'false'}, "
            f"{'true' if entry.skills else 'false'}, "
            f"{'true' if entry.has_config_schema else 'false'}, "
            f"{'true' if entry.websocket_capable else 'false'}, "
            f"{entry.config_key_count}"
            "},"
        )

    lines.extend(
        [
            "};",
            "",
            f"static constexpr size_t kOpenClawTranslatedExtensionCount = {len(entries)};",
            "",
            "}  // namespace octo::channels",
            "",
            "#endif  // OCTO_OPENCLAW_EXTENSION_CATALOG_GENERATED_H",
            "",
        ]
    )
    return "\n".join(lines)


def validate_entries(entries: list[ExtensionEntry]) -> list[str]:
    errors: list[str] = []
    for entry in entries:
        if entry.adapter_type == "channel" and entry.status == "implemented" and not entry.websocket_capable:
            errors.append(
                f"{entry.id}: channel 状态为 implemented，但 websocket_capable=false（违背 websocket-only 实施策略）"
            )
    return errors


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
    parser = argparse.ArgumentParser(description="翻译 OpenClaw 扩展到 ESP32 骨架")
    parser.add_argument(
        "--source-root",
        default=str(DEFAULT_SOURCE_ROOT),
        help="OpenClaw extensions 源目录",
    )
    parser.add_argument("--check", action="store_true", help="仅校验，不写文件")
    args = parser.parse_args()

    source_root = Path(args.source_root).resolve()
    if not source_root.exists():
        raise FileNotFoundError(f"源目录不存在: {source_root}")

    overrides = load_overrides()
    entries = load_entries(source_root, overrides)
    if not entries:
        raise RuntimeError("未找到任何 openclaw.plugin.json")

    validation_errors = validate_entries(entries)
    if validation_errors:
        for err in validation_errors:
            print(f"[ERROR] {err}")
        return 1

    ok = True
    for entry in entries:
        target_dir = CHANNELS_ROOT / entry.id
        ok &= write_or_check(
            target_dir / "extension_manifest.json",
            json.dumps(render_manifest_json(entry), ensure_ascii=False, indent=2) + "\n",
            args.check,
        )
        ok &= write_or_check(target_dir / "README.md", render_extension_readme(entry), args.check)

    ok &= write_or_check(DOC_MATRIX_PATH, render_matrix(entries), args.check)
    ok &= write_or_check(
        INDEX_JSON_PATH,
        json.dumps(render_index_json(entries), ensure_ascii=False, indent=2) + "\n",
        args.check,
    )
    ok &= write_or_check(GENERATED_HEADER_PATH, render_generated_header(entries), args.check)

    if not ok:
        return 1

    mode = "CHECK" if args.check else "WRITE"
    print(f"[{mode}] translated openclaw extensions: total={len(entries)}, source={source_root}, target={CHANNELS_ROOT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
