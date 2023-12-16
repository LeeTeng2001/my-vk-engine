#pragma once

#include "components/component.hpp"

class TweenComponent: public Component {
public:
    explicit TweenComponent(weak_ptr<Actor> owner, int updateOrder = 10);

    enum Movement {
        ETranslate // , ERotate, EScale
    };

    enum EaseType {
        EEaseLinear, EEaseInOutQuad
    };

    enum LoopType {
        EOneShot, ELoop
    };

    struct SeqBlock {
        Movement movementType;
        float durationS;
        function<void (float, float)> invokeF; // perc [0, 1], delta [0, 1]
    };

    void update(float deltaTime) override;

    void addTranslateOffset(float durS, glm::vec3 offSet, EaseType easeType = EEaseInOutQuad);

    // TODO: More animations

    void setEnable(bool enable) { _enable = enable; }
    void setLoopType(LoopType loopType) { _loopType = loopType; }

private:
    LoopType _loopType = ELoop;

    bool _enable = true;
    float _accumTimestampS{};
    int _curSeqBlock{};
    vector<unique_ptr<SeqBlock>> _animSeqList{};

    static float getEaseVal(EaseType type, float perc);
};
