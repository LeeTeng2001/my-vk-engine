#include "tween.hpp"
#include "actors/actor.hpp"
#include "glm/gtx/string_cast.hpp"

namespace luna {

TweenComponent::TweenComponent(const std::shared_ptr<Engine>& engine, int ownerId, int updateOrder)
    : Component(engine, ownerId, updateOrder) {}

void TweenComponent::update(float deltaTime) {
    if (!getEnabled() || _animSeqList.empty()) {
        return;
    }

    bool shouldAdvance = false;

    // clamp delta time
    if (_accumTimestampS + deltaTime >= _animSeqList[_curSeqBlock]->durationS) {
        deltaTime = _animSeqList[_curSeqBlock]->durationS - _accumTimestampS;
        shouldAdvance = true;
    }
    _accumTimestampS += deltaTime;

    // invoke anim
    _animSeqList[_curSeqBlock]->invokeF(_accumTimestampS / _animSeqList[_curSeqBlock]->durationS,
                                        deltaTime / _animSeqList[_curSeqBlock]->durationS);

    // advance next animation
    if (shouldAdvance) {
        // special ending condition
        if (_loopType == EOneShot && _curSeqBlock + 1 == _animSeqList.size()) {
            setEnable(false);
            _curSeqBlock = 0;
            _accumTimestampS = 0;
            return;
        }
        _accumTimestampS = 0;
        _curSeqBlock = (_curSeqBlock + 1) % _animSeqList.size();
    }
}

TweenComponent& TweenComponent::addTranslateOffset(float durS, glm::vec3 offSet,
                                                   EaseType easeType) {
    // TODO: validation duration
    std::unique_ptr<SeqBlock> seqPtr = std::make_unique<SeqBlock>();
    seqPtr->durationS = durS;
    seqPtr->invokeF = [this, offSet, easeType](float globalPerc, float stepDelta) {
        float actualPerc =
            getEaseVal(easeType, globalPerc) - getEaseVal(easeType, globalPerc - stepDelta);
        glm::vec3 pos = getOwner()->getLocalPosition();
        pos += offSet * actualPerc;
        getOwner()->setLocalPosition(pos);
    };
    _animSeqList.emplace_back(std::move(seqPtr));

    return *this;
}

TweenComponent& TweenComponent::addRotationOffset(float durS, float totalAngle,
                                                  const glm::vec3& axis, EaseType easeType) {
    // TODO: validation duration
    std::unique_ptr<SeqBlock> seqPtr = std::make_unique<SeqBlock>();
    seqPtr->durationS = durS;
    seqPtr->invokeF = [this, axis, totalAngle, easeType](float globalPerc, float stepDelta) {
        float actualPerc =
            getEaseVal(easeType, globalPerc) - getEaseVal(easeType, globalPerc - stepDelta);
        glm::quat rot = getOwner()->getRotation();
        rot *= glm::angleAxis(glm::radians(totalAngle) * actualPerc, axis);
        getOwner()->setRotation(rot);
    };
    _animSeqList.emplace_back(std::move(seqPtr));

    return *this;
}

float TweenComponent::getEaseVal(TweenComponent::EaseType type, float perc) {
    // https://easings.net/
    switch (type) {
        case EEaseLinear:
            return perc;
        case EEaseInOutQuad:
            return perc < 0.5f ? 2.0f * perc * perc
                               : 1.0f - glm::pow(-2.0f * perc + 2, 2.0f) / 2.0f;
    }
    SLog::get()->warn("unsupported ease type");
    return 0;
}

}  // namespace luna