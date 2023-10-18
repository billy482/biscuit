#include <QtCore/QCryptographicHash>
#include <QtCore/QIODevice>

#include "backup.hpp"
#include "../source/file-info.hpp"
#include "../source/source.hpp"

using namespace Biscuit::Worker;
using Biscuit::Source::FileInfo;
using Biscuit::Source::Source;

Backup::Backup() : QRunnable() {
	this->setAutoDelete(false);
}


void Backup::run() {
	for (Source::Source * source = Source::Source::first_source(); source != nullptr; source = source->next_source()) {
		for (FileInfo file_info = source->next(); not file_info.is_invalid(); file_info = source->next()) {
			// compare timestamp

			QIODevice * file_stream = source->open(file_info);

			QByteArray buffer = file_stream->read(4096);
			for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
				QByteArray digest = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);
				// find block into db and insert it if not found then retrieve its id

				buffer = file_stream->read(4096);
			}
		}
	}
}