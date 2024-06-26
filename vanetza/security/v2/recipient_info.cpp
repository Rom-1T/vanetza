#include <vanetza/security/v2/recipient_info.hpp>
#include <vanetza/security/v2/length_coding.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace vanetza
{
namespace security
{
namespace v2
{

PublicKeyAlgorithm get_type(const Key& key)
{
    struct key_visitor : public boost::static_visitor<PublicKeyAlgorithm>
    {
        PublicKeyAlgorithm operator()(const EciesEncryptedKey& key)
        {
            return PublicKeyAlgorithm::ECIES_NISTP256;
        }

        PublicKeyAlgorithm operator()(const OpaqueKey& key)
        {
            // TODO: could be anything except ECIES_NISTP256
            return PublicKeyAlgorithm::ECDSA_NISTP256_With_SHA256;
        }
    };

    key_visitor visitor;
    return boost::apply_visitor(visitor, key);
}

PublicKeyAlgorithm RecipientInfo::pk_encryption() const
{
    return get_type(enc_key);
}

size_t get_size(const RecipientInfo& info)
{
    size_t size = info.cert_id.size();
    size += sizeof(PublicKeyAlgorithm);
    struct RecipientInfoKey_visitor : public boost::static_visitor<size_t>
    {
        size_t operator()(const EciesEncryptedKey& key)
        {
            return key.c.size() + key.t.size() + get_size(key.v);
        }

        size_t operator()(const OpaqueKey& key)
        {
            return length_coding_size(key.data.size()) + key.data.size();
        }
    };

    RecipientInfoKey_visitor visit;
    size += boost::apply_visitor(visit, info.enc_key);
    return size;
}

void serialize(OutputArchive& ar, const RecipientInfo& info, SymmetricAlgorithm sym_algo)
{
    struct key_visitor : public boost::static_visitor<>
    {
        key_visitor(OutputArchive& ar, SymmetricAlgorithm sym_algo, PublicKeyAlgorithm pk_algo) :
            m_archive(ar), m_sym_algo(sym_algo), m_pk_algo(pk_algo)
        {
        }
        void operator()(const EciesEncryptedKey& key)
        {
            assert(key.c.size() == field_size(m_sym_algo));
            serialize(m_archive, key.v, m_pk_algo);
            for (auto& byte : key.c) {
                m_archive << byte;
            }
            for (auto& byte : key.t) {
                m_archive << byte;
            }
        }

        void operator()(const OpaqueKey& key)
        {
            serialize_length(m_archive, key.data.size());
            for (auto byte : key.data) {
                m_archive << byte;
            }
        }

        OutputArchive& m_archive;
        SymmetricAlgorithm m_sym_algo;
        PublicKeyAlgorithm m_pk_algo;
    };

    for (auto& byte : info.cert_id) {
        ar << byte;
    }
    const PublicKeyAlgorithm pk_algo = info.pk_encryption();
    serialize(ar, pk_algo);
    key_visitor visitor(ar, sym_algo, pk_algo);
    boost::apply_visitor(visitor, info.enc_key);
}

EciesEncryptedKey deserialize_ecies(InputArchive& ar, const SymmetricAlgorithm& symAlgo)
{
    EciesEncryptedKey ecies;
    deserialize(ar, ecies.v, PublicKeyAlgorithm::ECIES_NISTP256);
    const size_t fieldSize = field_size(symAlgo);
    for (size_t c = 0; c < fieldSize; ++c) {
        uint8_t tmp;
        ar >> tmp;
        ecies.c.push_back(tmp);
    }
    for (size_t c = 0; c < ecies.t.size(); ++c) {
        uint8_t tmp;
        ar >> tmp;
        ecies.t[c] = tmp;
    }
    return ecies;
}

OpaqueKey deserialize_opaque(InputArchive& ar)
{
    static const std::uintmax_t length_limit = 512;
    const std::uintmax_t length = deserialize_length(ar);

    if (length <= length_limit) {
        ByteBuffer opaque(length);
        for (std::uintmax_t i = 0; i < length; ++i) {
            ar >> opaque[i];
        }
        return OpaqueKey { std::move(opaque) };
    } else {
        return OpaqueKey {};
    }
}

size_t deserialize(InputArchive& ar, RecipientInfo& info, const SymmetricAlgorithm& symAlgo)
{
    for (size_t c = 0; c < info.cert_id.size(); ++c) {
        ar >> info.cert_id[c];
    }
    PublicKeyAlgorithm algo;
    deserialize(ar, algo);
    switch (algo) {
        case PublicKeyAlgorithm::ECIES_NISTP256:
            info.enc_key = deserialize_ecies(ar, symAlgo);
            break;
        default:
            info.enc_key = deserialize_opaque(ar);
            break;
    }
    return get_size(info);
}

} // namespace v2
} // namespace security
} // namespace vanetza
