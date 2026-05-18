import argparse
import os
import random
from typing import Dict, Iterable, Optional, Tuple

import pymysql
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad, unpad
import base64


local_table: Dict[str, int] = {}
key = get_random_bytes(16)
base_iv = get_random_bytes(16)


def AES_ENC(plaintext: bytes, iv: bytes) -> bytes:
    aes = AES.new(key, AES.MODE_CBC, iv=iv)
    padded_data = pad(plaintext, AES.block_size, style="pkcs7")
    return aes.encrypt(padded_data)


def AES_DEC(ciphertext: bytes, iv: bytes) -> bytes:
    aes = AES.new(key, AES.MODE_CBC, iv=iv)
    padded_data = aes.decrypt(ciphertext)
    return unpad(padded_data, AES.block_size, style="pkcs7")


def Random_Encrypt(plaintext: str) -> str:
    iv = get_random_bytes(16)
    ciphertext = AES_ENC(iv + AES_ENC(plaintext.encode("utf-8"), iv), base_iv)
    return base64.b64encode(ciphertext).decode("utf-8")


def Random_Decrypt(ciphertext: str) -> str:
    plaintext = AES_DEC(base64.b64decode(ciphertext.encode("utf-8")), base_iv)
    plaintext = AES_DEC(plaintext[16:], plaintext[:16])
    return plaintext.decode("utf-8")


def CalPos(plaintext: str, duplicate_policy: str = "random") -> int:
    presum = sum(v for k, v in local_table.items() if k < plaintext)
    old_count = local_table.get(plaintext, 0)
    new_count = old_count + 1
    local_table[plaintext] = new_count

    if old_count == 0:
        return presum
    if duplicate_policy == "left":
        return presum
    if duplicate_policy == "right":
        return presum + new_count - 1
    if duplicate_policy == "middle":
        return presum + new_count // 2
    return random.randint(presum, presum + new_count - 1)


def GetLeftPos(plaintext: str) -> int:
    return sum(v for k, v in local_table.items() if k < plaintext)


def GetRightPos(plaintext: str) -> int:
    return sum(v for k, v in local_table.items() if k <= plaintext)


def db_config() -> Dict[str, object]:
    return {
        "host": os.getenv("FH_DB_HOST", "localhost"),
        "port": int(os.getenv("FH_DB_PORT", "3306")),
        "user": os.getenv("FH_DB_USER", "user"),
        "password": os.getenv("FH_DB_PASSWORD", "123456"),
        "database": os.getenv("FH_DB_NAME", "test_db"),
        "charset": "utf8mb4",
        "autocommit": False,
    }


def connect():
    return pymysql.connect(**db_config())


def drain_cursor(cur) -> None:
    while cur.nextset():
        if cur.description:
            cur.fetchall()


def fetch_procedure_row(cur) -> Optional[Tuple[int, int, int]]:
    row = cur.fetchone() if cur.description else None
    while cur.nextset():
        if row is None and cur.description:
            row = cur.fetchone()
        elif cur.description:
            cur.fetchall()
    return row


def reset_experiment(conn) -> None:
    with conn.cursor() as cur:
        cur.execute("SELECT FHReset()")
        drain_cursor(cur)
        cur.execute("TRUNCATE TABLE example")
    conn.commit()
    local_table.clear()


def Insert(conn, plaintext: str, duplicate_policy: str = "random") -> Dict[str, int]:
    ciphertext = Random_Encrypt(plaintext)
    pos = CalPos(plaintext, duplicate_policy)

    with conn.cursor() as cur:
        cur.execute("CALL pro_insert(%s, %s)", (pos, ciphertext))
        row = fetch_procedure_row(cur)
    conn.commit()

    if row is None:
        row = (-1, -1, -1)
    return {
        "pos": pos,
        "inserted_encoding": int(row[0]),
        "update_start": int(row[1]),
        "update_end": int(row[2]),
    }


def Search(conn, left: str, right: str) -> None:
    left_pos = GetLeftPos(left)
    right_pos = GetRightPos(right)
    with conn.cursor() as cur:
        cur.execute(
            """
            SELECT ciphertext
            FROM example
            WHERE encoding >= FHSearch(%s)
              AND encoding < FHSearch(%s)
            ORDER BY encoding, id
            """,
            (left_pos, right_pos),
        )
        rest = cur.fetchall()

    for row in rest:
        ciphertext = row[0]
        print(f"ciphertext: {ciphertext} plaintext: {Random_Decrypt(ciphertext)}")


def fetch_snapshot(conn) -> Dict[str, int]:
    with conn.cursor() as cur:
        cur.execute("SELECT ciphertext, encoding FROM example")
        return {ciphertext: int(encoding) for ciphertext, encoding in cur.fetchall()}


def fetch_stats(conn) -> Dict[str, Optional[int]]:
    stats: Dict[str, Optional[int]] = {
        "rows": 0,
        "min_encoding": None,
        "max_encoding": None,
        "distinct_encoding": 0,
        "tree_total": None,
        "height": None,
        "leaf_count": None,
        "max_leaf_size": None,
    }
    with conn.cursor() as cur:
        cur.execute(
            "SELECT COUNT(*), MIN(encoding), MAX(encoding), COUNT(DISTINCT encoding) FROM example"
        )
        row = cur.fetchone()
        stats["rows"] = int(row[0])
        stats["min_encoding"] = None if row[1] is None else int(row[1])
        stats["max_encoding"] = None if row[2] is None else int(row[2])
        stats["distinct_encoding"] = int(row[3])

        cur.execute("SELECT FHTotalCount(), FHHeight(), FHLeafCount(), FHMaxLeafSize()")
        row = cur.fetchone()
        stats["tree_total"] = int(row[0])
        stats["height"] = int(row[1])
        stats["leaf_count"] = int(row[2])
        stats["max_leaf_size"] = int(row[3])
    return stats


def changed_encodings(before: Dict[str, int], after: Dict[str, int]) -> Dict[str, Tuple[int, int]]:
    changed = {}
    for ciphertext, old_encoding in before.items():
        new_encoding = after.get(ciphertext)
        if new_encoding is not None and new_encoding != old_encoding:
            changed[ciphertext] = (old_encoding, new_encoding)
    return changed


def fmt(value: Optional[int]) -> str:
    return "-" if value is None else str(value)


def run_demo(args) -> None:
    values = ["apple", "pear", "banana", "orange", "cherry", "apple", "cherry", "orange"]
    random.seed(args.seed)
    with connect() as conn:
        if args.reset:
            reset_experiment(conn)
        for plaintext in values:
            info = Insert(conn, plaintext, "random")
            print(
                f"insert plaintext={plaintext} pos={info['pos']} "
                f"encoding={info['inserted_encoding']} update=[{info['update_start']},{info['update_end']})"
            )

        print("\nsearch range [b, p]:")
        Search(conn, "b", "p")


def should_print(i: int, count: int, every: int, recode: bool, split: bool, show_first: int) -> bool:
    return i <= show_first or i == count or i % every == 0 or recode or split


def run_repeat(args) -> None:
    random.seed(args.seed)
    with connect() as conn:
        if args.reset:
            reset_experiment(conn)

        previous_snapshot = fetch_snapshot(conn)
        previous_stats = fetch_stats(conn)
        print(
            "i pos returned rows changed update_range height leaves max_leaf event"
        )

        for i in range(1, args.count + 1):
            info = Insert(conn, args.value, args.policy)
            snapshot = fetch_snapshot(conn)
            stats = fetch_stats(conn)
            changed = changed_encodings(previous_snapshot, snapshot)

            recode = info["inserted_encoding"] == 0 or info["update_start"] >= 0 or bool(changed)
            split = (
                previous_stats["leaf_count"] is not None
                and stats["leaf_count"] is not None
                and stats["leaf_count"] != previous_stats["leaf_count"]
            ) or (
                previous_stats["height"] is not None
                and stats["height"] is not None
                and stats["height"] != previous_stats["height"]
            )

            if should_print(i, args.count, args.every, recode, split, args.show_first):
                events = []
                if recode:
                    events.append("recode")
                if split:
                    events.append("split")
                event_text = ",".join(events) if events else "-"
                print(
                    f"{i} {info['pos']} {info['inserted_encoding']} {stats['rows']} "
                    f"{len(changed)} [{info['update_start']},{info['update_end']}) "
                    f"{fmt(stats['height'])} {fmt(stats['leaf_count'])} "
                    f"{fmt(stats['max_leaf_size'])} {event_text}"
                )

                if changed and args.show_changes > 0:
                    sample = list(changed.values())[: args.show_changes]
                    sample_text = ", ".join(f"{old}->{new}" for old, new in sample)
                    print(f"  changed sample: {sample_text}")

            previous_snapshot = snapshot
            previous_stats = stats

        print("\nfinal stats:")
        final_stats = fetch_stats(conn)
        for key, value in final_stats.items():
            print(f"{key}: {fmt(value)}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="FH-OPE client and repeat-insert experiment.")
    subparsers = parser.add_subparsers(dest="command")

    demo = subparsers.add_parser("demo", help="run the original guide example")
    demo.add_argument("--seed", type=int, default=2026)
    demo.add_argument("--no-reset", dest="reset", action="store_false")
    demo.set_defaults(reset=True, func=run_demo)

    repeat = subparsers.add_parser("repeat", help="insert the same plaintext many times")
    repeat.add_argument("--value", default="apple")
    repeat.add_argument("--count", type=int, default=140)
    repeat.add_argument("--policy", choices=["left", "right", "middle", "random"], default="left")
    repeat.add_argument("--seed", type=int, default=2026)
    repeat.add_argument("--every", type=int, default=10)
    repeat.add_argument("--show-first", type=int, default=5)
    repeat.add_argument("--show-changes", type=int, default=3)
    repeat.add_argument("--no-reset", dest="reset", action="store_false")
    repeat.set_defaults(reset=True, func=run_repeat)

    parser.set_defaults(func=run_demo, command="demo", reset=True, seed=2026)
    return parser


if __name__ == "__main__":
    parsed_args = build_parser().parse_args()
    parsed_args.func(parsed_args)
