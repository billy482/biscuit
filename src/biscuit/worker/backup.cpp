#include <QtCore/QCryptographicHash>

#include "backup.hpp"
#include "../source.hpp"

using namespace Biscuit::Worker;
using Biscuit::Source;

Backup::Backup() : QRunnable() {
	this->setAutoDelete(false);
}


void Backup::run() {
	QList<Source>& sources = Source::get();
	for (Source& source : sources) {
		for (QFileInfo file_info = source.next(); file_info.exists(); file_info = source.next()) {
			// compare timestamp

			QIODevice * file_stream = source.open(file_info);

			QByteArray buffer = file_stream->read(4096);
			for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
				QByteArray digest = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);
				// find block into db and insert it if not found then retrieve its id

				buffer = file_stream->read(4096);
			}
		}
	}
}