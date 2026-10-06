#include "ErgonomicTipCatalog.h"
#include <QRandomGenerator>

static const QList<ErgonomicTip> s_catalog = {
    {
        "EYE RELIEF",
        "The 20-20-20 Rule",
        "Gaze at an object at least 20 feet (6 meters) away to completely relax your eye's focusing muscles.",
        "Releases ciliary muscle tension and prevents accommodative fatigue.",
        false
    },
    {
        "DRY EYE RESET",
        "Deliberate Blink Cycle",
        "Close your eyes normally for 2 seconds, open, then close gently and squeeze tightly for 2 seconds. Repeat 5 times.",
        "Reactivates meibomian gland oil flow and coats the cornea with fresh tears.",
        false
    },
    {
        "FOCUS MUSCLES",
        "Figure Eight Smooth Tracking",
        "Imagine a giant figure 8 on a distant wall 10 feet away. Trace it smoothly with your eyes without moving your head for 15 seconds.",
        "Enhances extraocular muscle coordination and relieves directional screen lock.",
        false
    },
    {
        "FOCUS MUSCLES",
        "Near-Far Pencil Push-Up",
        "Hold your thumb 10 inches from your nose. Focus on it for 3 seconds, then shift focus to a distant horizon for 3 seconds. Repeat 4 times.",
        "Maintains flexibility of the crystalline lens and prevents convergence insufficiency.",
        false
    },
    {
        "EYE RELIEF",
        "Palming Warmth Soothing",
        "Rub your palms together vigorously until warm. Cup your palms gently over closed eyes without pressing the eyelids. Breathe slowly in total dark.",
        "Calms the optic nerve through warmth and sensory darkness rest.",
        false
    },
    {
        "POSTURE & SPINE",
        "Chin Tucks & Neck Retraction",
        "Sit tall. Look straight ahead, gently tuck your chin backward making a slight 'double chin'. Hold for 5 seconds, release, and repeat 5 times.",
        "Counters forward head posture ('tech neck') and decompresses suboccipital nerves.",
        true
    },
    {
        "POSTURE & SPINE",
        "Doorway / Wall Chest Opener",
        "Place your forearms against a doorframe at 90 degrees. Step forward gently until you feel a comfortable stretch across your chest. Hold for 20 seconds.",
        "Reverses hunched shoulder posture and reduces upper thoracic strain.",
        true
    },
    {
        "POSTURE & SPINE",
        "Seated Spinal Twist",
        "Sit upright with feet flat. Place your right hand on your left knee and gently rotate your torso to the left. Hold 15s, then switch sides.",
        "Improves spinal mobility, stimulates circulation, and relieves lower back compression.",
        true
    },
    {
        "POSTURE & SPINE",
        "Shoulder Blade Scapular Squeeze",
        "Pull your shoulder blades backward and downward as if squeezing a pencil between them. Hold for 5 seconds, release, and repeat 6 times.",
        "Strengthens rhomboids and relieves trapezius tension from keyboard use.",
        true
    },
    {
        "POSTURE & SPINE",
        "Wrist Flexor & Extensor Stretch",
        "Extend your right arm forward with palm facing up. Use your left hand to gently pull fingers down and back. Hold 10s, then flip palm down and hold 10s. Switch arms.",
        "Decompresses the carpal tunnel and prevents repetitive strain injury (RSI).",
        true
    }
};

const QList<ErgonomicTip>& ErgonomicTipCatalog::allTips() {
    return s_catalog;
}

ErgonomicTip ErgonomicTipCatalog::getRandomTip(bool preferMacro) {
    QList<ErgonomicTip> filtered;
    for (const auto& tip : s_catalog) {
        if (tip.isMacro == preferMacro) {
            filtered.append(tip);
        }
    }
    if (filtered.isEmpty()) {
        filtered = s_catalog;
    }
    int idx = QRandomGenerator::global()->bounded(filtered.size());
    return filtered.at(idx);
}

ErgonomicTip ErgonomicTipCatalog::getNextTip(const QString& currentTitle, bool preferMacro) {
    QList<ErgonomicTip> pool;
    for (const auto& tip : s_catalog) {
        if (tip.isMacro == preferMacro) {
            pool.append(tip);
        }
    }
    if (pool.isEmpty()) {
        pool = s_catalog;
    }

    int currentIdx = -1;
    for (int i = 0; i < pool.size(); ++i) {
        if (pool[i].title == currentTitle) {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx == -1) {
        return pool.first();
    }
    int nextIdx = (currentIdx + 1) % pool.size();
    return pool.at(nextIdx);
}
