#include <vanetza/security/sign_service.hpp>
#include <vanetza/security/v3/signer_info.hpp>
#include <vanetza/security/v3/certificate_provider.hpp>
#include <vanetza/security/v3/sign_header_policy.hpp>
#include <vanetza/security/v3/secured_message.hpp>

namespace vanetza
{
namespace security
{

// forward declarations
class Backend;

namespace v3
{

/**
 * SignService immediately signing the message using given
 */
class StraightSignService : public SignService
{
public:
   StraightSignService(CertificateProvider&, Backend&, SignHeaderPolicy&);
   SignConfirm sign(SignRequest&&) override;

private:
    CertificateProvider & m_certificates;
    Backend& m_backend;
    SignHeaderPolicy& m_policy;
};
    

/**
 * SignService without real cryptography but dummy signature
 */
class DummySignService : public SignService
{
public:
    /**
     * \param rt runtime for appropriate generation time
     * \param si signer info attached to header fields of secured message
     */ 
    DummySignService(const Runtime& rt, const SignerInfo& si);
    SignConfirm sign(SignRequest&&) override;

private:
    const Runtime& m_runtime;
    SignerInfo m_signer_info;
};

} // namespace v3
} // namespace security
} // namespace vanetza
