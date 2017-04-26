#include "engine/effects/engineeffect.h"

#include "util/sample.h"

EngineEffect::EngineEffect(EffectManifestPointer pManifest,
                           const QSet<ChannelHandleAndGroup>& registeredChannels,
                           EffectInstantiatorPointer pInstantiator)
        : m_pManifest(pManifest),
          m_enableState(EffectProcessor::DISABLED),
          m_parameters(pManifest->parameters().size()) {
    const QList<EffectManifestParameterPointer>& parameters = m_pManifest->parameters();
    for (int i = 0; i < parameters.size(); ++i) {
        EffectManifestParameterPointer param = parameters.at(i);
        EngineEffectParameter* pParameter =
                new EngineEffectParameter(param);
        m_parameters[i] = pParameter;
        m_parametersById[param->id()] = pParameter;
    }

    // Creating the processor must come last.
    m_pProcessor = pInstantiator->instantiate(this, pManifest);
    m_pProcessor->initialize(registeredChannels);
    m_effectRampsFromDry = pManifest->effectRampsFromDry();
}

EngineEffect::~EngineEffect() {
    if (kEffectDebugOutput) {
        qDebug() << debugString() << "destroyed";
    }
    delete m_pProcessor;
    m_parametersById.clear();
    for (int i = 0; i < m_parameters.size(); ++i) {
        EngineEffectParameter* pParameter = m_parameters.at(i);
        m_parameters[i] = NULL;
        delete pParameter;
    }
}

bool EngineEffect::processEffectsRequest(const EffectsRequest& message,
                                         EffectsResponsePipe* pResponsePipe) {
    EngineEffectParameter* pParameter = NULL;
    EffectsResponse response(message);

    switch (message.type) {
        case EffectsRequest::SET_EFFECT_PARAMETERS:
            if (kEffectDebugOutput) {
                qDebug() << debugString() << "SET_EFFECT_PARAMETERS"
                         << "enabled" << message.SetEffectParameters.enabled;
            }

            if (m_enableState != EffectProcessor::DISABLED && !message.SetEffectParameters.enabled) {
                m_enableState = EffectProcessor::DISABLING;
            } else if (m_enableState == EffectProcessor::DISABLED && message.SetEffectParameters.enabled) {
                m_enableState = EffectProcessor::ENABLING;
            }

            response.success = true;
            pResponsePipe->writeMessages(&response, 1);
            return true;
            break;
        case EffectsRequest::SET_PARAMETER_PARAMETERS:
            if (kEffectDebugOutput) {
                qDebug() << debugString() << "SET_PARAMETER_PARAMETERS"
                         << "parameter" << message.SetParameterParameters.iParameter
                         << "minimum" << message.minimum
                         << "maximum" << message.maximum
                         << "default_value" << message.default_value
                         << "value" << message.value;
            }
            pParameter = m_parameters.value(
                message.SetParameterParameters.iParameter, NULL);
            if (pParameter) {
                pParameter->setMinimum(message.minimum);
                pParameter->setMaximum(message.maximum);
                pParameter->setDefaultValue(message.default_value);
                pParameter->setValue(message.value);
                response.success = true;
            } else {
                response.success = false;
                response.status = EffectsResponse::NO_SUCH_PARAMETER;
            }
            pResponsePipe->writeMessages(&response, 1);
            return true;
        default:
            break;
    }
    return false;
}

void EngineEffect::process(const ChannelHandle& handle,
                           const CSAMPLE* pInput, CSAMPLE* pOutput,
                           const unsigned int numSamples,
                           const unsigned int sampleRate,
                           const EffectProcessor::EnableState enableState,
                           const GroupFeatureState& groupFeatures) {
    EffectProcessor::EnableState effectiveEnableState = m_enableState;
    if (enableState == EffectProcessor::DISABLING) {
        effectiveEnableState = EffectProcessor::DISABLING;
    } else if (enableState == EffectProcessor::ENABLING) {
        effectiveEnableState = EffectProcessor::ENABLING;
    }

    m_pProcessor->process(handle, pInput, pOutput, numSamples, sampleRate,
            effectiveEnableState, groupFeatures);
    if (!m_effectRampsFromDry) {
        // the effect does not fade, so we care for it
        if (effectiveEnableState == EffectProcessor::DISABLING) {
            DEBUG_ASSERT(pInput != pOutput); // Fade to dry only works if pInput is not touched by pOutput
            // Fade out (fade to dry signal)
            SampleUtil::copy2WithRampingGain(pOutput,
                    pInput, 0.0, 1.0,
                    pOutput, 1.0, 0.0,
                    numSamples);
        } else if (effectiveEnableState == EffectProcessor::ENABLING) {
            DEBUG_ASSERT(pInput != pOutput); // Fade to dry only works if pInput is not touched by pOutput
            // Fade in (fade to wet signal)
            SampleUtil::copy2WithRampingGain(pOutput,
                    pInput, 1.0, 0.0,
                    pOutput, 0.0, 1.0,
                    numSamples);
        }
    }

    if (m_enableState == EffectProcessor::DISABLING) {
        m_enableState = EffectProcessor::DISABLED;
    } else if (m_enableState == EffectProcessor::ENABLING) {
        m_enableState = EffectProcessor::ENABLED;
    }
}
