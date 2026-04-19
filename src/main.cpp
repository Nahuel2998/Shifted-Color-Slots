#include <Geode/Geode.hpp>
#include <Geode/binding/CustomizeObjectLayer.hpp>
#include <Geode/modify/CustomizeObjectLayer.hpp>

using namespace geode::prelude;

#define CUSTOM_CHANNEL_ID 1008

class $modify(CustomizeObjectLayer) {
    struct Fields {
        // Disables updating the selected button sprite and custom color labels
        bool m_skipVanillaCustomHandling = false;
    };

    bool
    init(GameObject *object, cocos2d::CCArray *objects) {
		if (!CustomizeObjectLayer::init(object, objects)) {
			return false;
		}

        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset) return true;

        auto channelsMenu = getChildByIDRecursive("channels-menu");
        if (!channelsMenu) return true; // Should be unreachable...

        for (int tag = 1; tag < 10; tag++) {
            auto button = channelsMenu->getChildByTag(tag);
            auto sprite = static_cast<ButtonSprite*>(button->getChildByTag(tag));
            if (!sprite) continue;

            int newtag = tag + offset;
            button->setTag(newtag);
            sprite->setTag(newtag);

            if (Mod::get()->getSettingValue<bool>("update-labels")) {
                sprite->setString(std::to_string(newtag).c_str());
            }
        }

        updateCurrentSelection();
        return true;
    }

    void
    updateCurrentSelection() {
        auto offset = Mod::get()->getSettingValue<int>("offset");

        int tag = getActiveMode(false);
        if (!isInOffsetRange(offset, tag) && 0 < tag && tag < 1000) {
            updateCustomColorChannel(tag);
            tag = CUSTOM_CHANNEL_ID;
        }
        ButtonSprite *sprite = getButtonByTag(tag);
        highlightSelected(sprite);
    }

    // -- Functions that have updateCurrentSelection() inlined
    // These functions internally check (id >= 10) for determining a custom color
    // and thus update m_customColorChannel and call updateCustomColorLabels() and highlightSelected()
    // When they're called, we want to skip those updates and call updateCurrentSelection() ourselves after
    // TODO: It's also inlined in onNextColorChannel() [Next Free]
    void
    toggleVisible() {
        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset) {
            CustomizeObjectLayer::toggleVisible();
            return;
        }

        int prevCustomColorChannel = m_customColorChannel;
        m_fields->m_skipVanillaCustomHandling = true;
        {
            CustomizeObjectLayer::toggleVisible();
        }
        m_fields->m_skipVanillaCustomHandling = false;
        m_customColorChannel = prevCustomColorChannel;

        updateCurrentSelection();
    }

    void
    colorSetupClosed(int id) {
        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset) {
            CustomizeObjectLayer::colorSetupClosed(id);
            return;
        }

        int prevCustomColorChannel = m_customColorChannel;
        m_fields->m_skipVanillaCustomHandling = true;
        {
            CustomizeObjectLayer::colorSetupClosed(id);
        }
        m_fields->m_skipVanillaCustomHandling = false;
        m_customColorChannel = prevCustomColorChannel;

        updateCurrentSelection();
    }

    // -- Functions guarded by skipVanillaCustomHandling
    void
    updateCustomColorLabels() {
        if (m_fields->m_skipVanillaCustomHandling) return;
        CustomizeObjectLayer::updateCustomColorLabels();
    }

    void
    highlightSelected(ButtonSprite *sprite) {
        if (m_fields->m_skipVanillaCustomHandling) return;
        CustomizeObjectLayer::highlightSelected(sprite);
    }

    // -- Utils
    inline void
    updateCustomColorChannel(int newid) {
        m_disableTextDelegate = true;
        m_customColorChannel  = newid;
        updateCustomColorLabels();
        m_disableTextDelegate = false;
    }

    static inline bool
    isInOffsetRange(int offset, int id) { return offset < id && id < offset + 10; }
};
