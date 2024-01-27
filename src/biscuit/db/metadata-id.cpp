#include "metadata-id.hpp"

using namespace Biscuit::Db;

MetadataId::MetadataId(SqlStatus status) : SqlResult(status) {}

MetadataId::MetadataId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

MetadataId::MetadataId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

MetadataId::MetadataId(const MetadataId& metadata_id) : SqlResult(metadata_id) {}

MetadataId::MetadataId(MetadataId&& metadata_id) : SqlResult(metadata_id) {}


MetadataId& MetadataId::operator=(const MetadataId& metadata_id) {
	this->copy(metadata_id);
	return *this;
}

MetadataId& MetadataId::operator=(MetadataId&& metadata_id) {
	this->move(std::move(metadata_id));
	return *this;
}
