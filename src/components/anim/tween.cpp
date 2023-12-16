#include <utility>

#include "tween.hpp"
#include "actors/actor.hpp"

TweenComponent::TweenComponent(weak_ptr<Actor> owner, int updateOrder) : Component(std::move(owner), updateOrder) {

}

void TweenComponent::update(float deltaTime) {
    if (!_enable || _animSeqList.size() == 0) {
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
    _animSeqList[_curSeqBlock]->invokeF(_accumTimestampS / _animSeqList[_curSeqBlock]->durationS, deltaTime / _animSeqList[_curSeqBlock]->durationS);

    // advance next animation
    if (shouldAdvance) {
        // special ending condition
        if (_loopType == EOneShot && _curSeqBlock + 1 == _animSeqList.size()) {
            _enable = false;
            _curSeqBlock = 0;
            _accumTimestampS = 0;
            return;
        }
        _accumTimestampS = 0;
        _curSeqBlock = (_curSeqBlock + 1) % _animSeqList.size();
    }
}

void TweenComponent::addTranslateOffset(float durS, glm::vec3 offSet, EaseType easeType) {
    // TODO: validation duration
    unique_ptr<SeqBlock> seqPtr = make_unique<SeqBlock>();
    seqPtr->movementType = ETranslate;
    seqPtr->durationS = durS;
    seqPtr->invokeF = [this, offSet, easeType](float globalPerc, float stepDelta) {
        float actualPerc = getEaseVal(easeType, globalPerc) - getEaseVal(easeType, globalPerc - stepDelta);
        glm::vec3 pos = getOwner()->getPosition();
        pos += offSet * actualPerc;
        getOwner()->setPosition(pos);
    };
    _animSeqList.emplace_back(std::move(seqPtr));
}

float TweenComponent::getEaseVal(TweenComponent::EaseType type, float perc) {
    // https://easings.net/
    switch (type) {
        case EEaseLinear:
            return perc;
        case EEaseInOutQuad:
            return perc < 0.5 ? 2 * perc * perc : 1 - glm::pow(-2 * perc + 2, 2) / 2.0f;
    }
}
