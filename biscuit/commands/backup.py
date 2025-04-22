import argparse
from typing import Dict


def _backup(args: argparse.Namespace, config: Dict) -> int:
	return 1


def backup_parse(sub_parser: argparse._SubParsersAction) -> None:
	parser = sub_parser.add_parser('backup', help="backup files")
	parser.set_defaults(func=_backup)
