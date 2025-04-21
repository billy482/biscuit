import argparse


def _backup(args, config):
	return

def backup_parse(sub_parser: argparse._SubParsersAction):
	parser = sub_parser.add_parser('backup', help="backup files")
	parser.set_defaults(func=_backup)
