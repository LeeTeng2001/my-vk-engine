#pragma once

#include "components/component.hpp"

class TweenComponent: public Component {
public:
    explicit TweenComponent(weak_ptr<Actor> owner, int updateOrder = 10);

    enum EaseType {
        EEaseLinear, EEaseInOutQuad
    };

    enum LoopType {
        EOneShot, ELoop
    };

    struct SeqBlock {
        float durationS;
        function<void (float, float)> invokeF; // perc [0, 1], delta [0, 1]
    };

    void update(float deltaTime) override;

    TweenComponent& addTranslateOffset(float durS, glm::vec3 offSet, EaseType easeType = EEaseInOutQuad);
    TweenComponent& addYRotationOffset(float durS, float totalAngle, EaseType easeType = EEaseLinear);
    TweenComponent& setLoopType(LoopType loopType) { _loopType = loopType; return *this; }

    // TODO: More animations


private:
    LoopType _loopType = ELoop;

    float _accumTimestampS{};
    int _curSeqBlock{};
    vector<unique_ptr<SeqBlock>> _animSeqList{};

    static float getEaseVal(EaseType type, float perc);
};
