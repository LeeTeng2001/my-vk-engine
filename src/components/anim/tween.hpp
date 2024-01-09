#pragma once

#include "components/component.hpp"

// Provide tweening capability
// for actor properties

namespace luna {

class TweenComponent: public Component {
public:
    explicit TweenComponent(const std::shared_ptr<Engine> &engine, int ownerId, int updateOrder = 10);

    enum EaseType {
        EEaseLinear, EEaseInOutQuad
    };

    enum LoopType {
        EOneShot, ELoop
    };

    struct SeqBlock {
        float durationS;
        std::function<void (float, float)> invokeF; // perc [0, 1], delta [0, 1]
    };

    void update(float deltaTime) override;

    TweenComponent& addTranslateOffset(float durS, glm::vec3 offSet, EaseType easeType = EEaseInOutQuad);
    TweenComponent& addRotationOffset(float durS, float totalAngle, const glm::vec3 &axis, EaseType easeType = EEaseLinear);
    TweenComponent& setLoopType(LoopType loopType) { _loopType = loopType; return *this; }

    // TODO: More animations


private:
    LoopType _loopType = ELoop;

    float _accumTimestampS{};
    int _curSeqBlock{};
    std::vector<std::unique_ptr<SeqBlock>> _animSeqList{};

    static float getEaseVal(EaseType type, float perc);
};

}