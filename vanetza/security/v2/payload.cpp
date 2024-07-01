#include <vanetza/security/v2/payload.hpp>
#include <vanetza/security/v2/length_coding.hpp>

namespace vanetza
{
namespace security
{
namespace v2
{

PayloadType get_type(const Payload& payload)
{
    return payload.type;
}

size_t get_size(const Payload& payload)
{
    size_t length = sizeof(PayloadType);
    const size_t data = size(payload.data, OsiLayer::Network, max_osi_layer());
    length += data;
    length += length_coding_size(data);
    return length;
}

size_t get_size(const ByteBuffer& buf)
{
    size_t size = buf.size();
    size += length_coding_size(size);
    return size;
}

void serialize(OutputArchive& ar, const Payload& payload)
{
    serialize(ar, payload.type);
    serialize_length(ar, size(payload.data, OsiLayer::Network, max_osi_layer()));
    serialize(ar, payload.data);
}

size_t deserialize(InputArchive& ar, Payload& payload)
{
    size_t size = sizeof(PayloadType);
    PayloadType type;
    deserialize(ar, type);
    payload.type = type;

    static const std::size_t data_length_limit = 4096;
    const auto data_length = deserialize_length(ar);
    if (data_length <= data_length_limit) {
        size += length_coding_size(data_length);
        size += data_length;
        ByteBuffer buf(data_length);
        ar.load_binary(buf.data(), buf.size());
        payload.data = CohesivePacket(std::move(buf), OsiLayer::Network);
    } else {
        ar.fail(InputArchive::ErrorCode::ExcessiveLength);
    }

    return size;
}

} // namespace v2
} // namespace security
} // namespace vanetza
