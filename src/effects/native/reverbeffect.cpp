#include "effects/native/reverbeffect.h"

#include <QtDebug>

#include "util/sample.h"

// static
QString ReverbEffect::getId() {
    return "org.mixxx.effects.reverb";
}

// static
EffectManifestPointer ReverbEffect::getManifest() {
    EffectManifestPointer pManifest(new EffectManifest());
    pManifest->setId(getId());
    pManifest->setName(QObject::tr("Reverb"));
    pManifest->setAuthor("The Mixxx Team, CAPS Plugins");
    pManifest->setVersion("1.0");
    pManifest->setDescription("This is a port of the GPL'ed CAPS Reverb plugin, "
            "which has the following description:"
            "This is based on some of the famous Stanford CCRMA reverbs "
            "(NRev, KipRev) all based on the Chowning/Moorer/Schroeder "
            "reverberators, which use networks of simple allpass and comb"
            "delay filters.");
    pManifest->setEffectRampsFromDry(true);

    EffectManifestParameterPointer decay = pManifest->addParameter();
    decay->setId("decay");
    decay->setName(QObject::tr("Decay"));
    decay->setDescription(QObject::tr("Lower decay values cause reverberations to die out more quickly."));
    decay->setControlHint(EffectManifestParameter::ControlHint::KNOB_LINEAR);
    decay->setSemanticHint(EffectManifestParameter::SemanticHint::UNKNOWN);
    decay->setUnitsHint(EffectManifestParameter::UnitsHint::UNKNOWN);
    decay->setMinimum(0);
    decay->setDefault(0.5);
    decay->setMaximum(1);

    EffectManifestParameterPointer bandwidth = pManifest->addParameter();
    bandwidth->setId("bandwidth");
    bandwidth->setName(QObject::tr("Bandwidth"));
    bandwidth->setShortName(QObject::tr("BW"));
    bandwidth->setDescription(QObject::tr("Bandwidth of the low pass filter at the input. "
            "Higher values result in less attenuation of high frequencies."));
    bandwidth->setControlHint(EffectManifestParameter::ControlHint::KNOB_LINEAR);
    bandwidth->setSemanticHint(EffectManifestParameter::SemanticHint::UNKNOWN);
    bandwidth->setUnitsHint(EffectManifestParameter::UnitsHint::UNKNOWN);
    bandwidth->setMinimum(0);
    bandwidth->setDefault(1);
    bandwidth->setMaximum(1);

    EffectManifestParameterPointer damping = pManifest->addParameter();
    damping->setId("damping");
    damping->setName(QObject::tr("Damping"));
    damping->setDescription(QObject::tr("Higher damping values cause "
            "high frequencies to decay more quickly than low frequencies."));
    damping->setControlHint(EffectManifestParameter::ControlHint::KNOB_LINEAR);
    damping->setSemanticHint(EffectManifestParameter::SemanticHint::UNKNOWN);
    damping->setUnitsHint(EffectManifestParameter::UnitsHint::UNKNOWN);
    damping->setMinimum(0);
    damping->setDefault(0);
    damping->setMaximum(1);

    EffectManifestParameterPointer send = pManifest->addParameter();
    send->setId("send_amount");
    send->setName(QObject::tr("Send"));
    send->setDescription(QObject::tr("How much of the signal to send to the effect"));
    send->setControlHint(EffectManifestParameter::ControlHint::KNOB_LINEAR);
    send->setSemanticHint(EffectManifestParameter::SemanticHint::UNKNOWN);
    send->setUnitsHint(EffectManifestParameter::UnitsHint::UNKNOWN);
    send->setDefaultLinkType(EffectManifestParameter::LinkType::LINKED);
    send->setDefaultLinkInversion(EffectManifestParameter::LinkInversion::NOT_INVERTED);
    send->setMinimum(0);
    send->setDefault(0);
    send->setMaximum(1);
    return pManifest;
}

ReverbEffect::ReverbEffect(EngineEffect* pEffect)
        : m_pDecayParameter(pEffect->getParameterById("decay")),
          m_pBandWidthParameter(pEffect->getParameterById("bandwidth")),
          m_pDampingParameter(pEffect->getParameterById("damping")),
          m_pSendParameter(pEffect->getParameterById("send_amount")) {
}

ReverbEffect::~ReverbEffect() {
    //qDebug() << debugString() << "destroyed";
}

void ReverbEffect::processChannel(const ChannelHandle& handle,
                                ReverbGroupState* pState,
                                const CSAMPLE* pInput, CSAMPLE* pOutput,
                                const unsigned int numSamples,
                                const unsigned int sampleRate,
                                const EffectProcessor::EnableState enableState,
                                const GroupFeatureState& groupFeatures) {
    Q_UNUSED(handle);
    Q_UNUSED(enableState);
    Q_UNUSED(groupFeatures);

    if (!pState || !m_pDecayParameter || !m_pBandWidthParameter || !m_pDampingParameter || !m_pSendParameter) {
        qWarning() << "Could not retrieve all effect parameters";
        return;
    }

    const auto decay = m_pDecayParameter->value();
    const auto bandwidth = m_pBandWidthParameter->value();
    const auto damping = m_pDampingParameter->value();
    const auto send = m_pSendParameter->value();

    if (pState->sampleRate != sampleRate) {
        // update the sample rate if it's changed
        pState->reverb.init(sampleRate);
        pState->sampleRate = sampleRate;
    }
    pState->reverb.processBuffer(pInput, pOutput, numSamples, bandwidth, decay, damping, send);
}
