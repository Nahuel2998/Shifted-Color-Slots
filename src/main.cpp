#include <Geode/Geode.hpp>
#include <Geode/modify/CustomizeObjectLayer.hpp>

using namespace geode::prelude;

#define CUSTOM_CHANNEL_ID 1008

static constexpr std::string_view BUTTON_IDS[] = {
    "channel-1-button",
    "channel-2-button",
    "channel-3-button",
    "channel-4-button",
    "channel-5-button",
    "channel-6-button",
    "channel-7-button",
    "channel-8-button",
    "channel-9-button",
};

class $modify(WithShiftedColorsCustomizeObjectLayer, CustomizeObjectLayer) {
    struct Fields {
        // Disables updating the selected button sprite and custom color labels
        bool       m_skipVanillaCustomHandling = false;
        TextInput *m_shiftOffsetInput;
    };

    $override
    bool
    init(GameObject *object, cocos2d::CCArray *objects) {
		if (!CustomizeObjectLayer::init(object, objects)) return false;
        if (isBetterMenuRunning()) return true;

        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (Mod::get()->getSettingValue<bool>("in-editor-menu")) {
            setupColorShiftMenu(offset);
        }

        if (offset) updateButtonTags(offset);
        return true;
    }

    bool
    updateButtonTags(int offset) {
        auto channelsMenu = m_mainLayer->getChildByID("channels-menu");
        if (!channelsMenu) return false; // Should be unreachable...

        for (int tag = 1; tag < 10; tag++) {
            auto button = channelsMenu->getChildByID(BUTTON_IDS[tag - 1]);
            auto sprite = static_cast<ButtonSprite*>(button->getChildByTag(button->getTag()));
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

    $override
    void
    updateCurrentSelection() {
        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset || isBetterMenuRunning()) {
            CustomizeObjectLayer::updateCurrentSelection();
            return;
        }

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
    $override
    void
    toggleVisible() {
        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset || isBetterMenuRunning()) {
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

    $override
    void
    colorSetupClosed(int id) {
        auto offset = Mod::get()->getSettingValue<int>("offset");
        if (!offset || isBetterMenuRunning()) {
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
    $override
    void
    updateCustomColorLabels() {
        if (m_fields->m_skipVanillaCustomHandling) return;
        CustomizeObjectLayer::updateCustomColorLabels();
    }

    $override
    void
    highlightSelected(ButtonSprite *sprite) {
        if (m_fields->m_skipVanillaCustomHandling) return;
        CustomizeObjectLayer::highlightSelected(sprite);
    }

    // -- In-Editor Menu
    void
    setupColorShiftMenu(int initialOffset) {
        auto alertBg = m_mainLayer->getChildByID("alert-bg");
        if (!alertBg) return;

        auto menu = CCMenu::create();
        menu->setID("color-shift-menu"_spr);

        auto incrBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png"),
            this,
            menu_selector(WithShiftedColorsCustomizeObjectLayer::onShiftOffsetButton)
        );
        incrBtn->setRotation(90);
        incrBtn->setTag(+1);

        auto decrBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png"),
            this,
            menu_selector(WithShiftedColorsCustomizeObjectLayer::onShiftOffsetButton)
        );
        decrBtn->setRotation(90);
        decrBtn->setTag(-1);

        auto width = incrBtn->getContentWidth() * 2;

        auto input = TextInput::create(width, "0");
        input->setCommonFilter(CommonFilter::Uint);
        input->setMaxCharCount(3);
        input->setString(std::to_string(initialOffset).c_str());
        input->setCallback([this](const std::string &text) {
            int offset = 0;
            if (text.length() != 0) {
                auto res = numFromString<int>(text);
                offset   = res.unwrapOrDefault();
            }
            setShiftOffset(offset);
        });
        m_fields->m_shiftOffsetInput = input;

        menu->addChild(incrBtn);
        menu->addChild(input);
        menu->addChild(decrBtn);

        m_mainLayer->addChild(menu);

        auto posX = alertBg->getPositionX() - alertBg->getContentWidth() / 2;
        menu->setPositionX(posX);
        menu->setAnchorPoint({1, 0.5});
        menu->setContentWidth(width * 1.25);

        menu->setLayout(SimpleColumnLayout::create());
    }

    void
    onShiftOffsetButton(CCObject *sender) { setShiftOffset(sender->getTag(), true); }

    int
    setShiftOffset(int newOffset, bool relative = false) {
        auto setting = std::static_pointer_cast<SettingTypeForValueType<int>::SettingType>(Mod::get()->getSetting("offset"));
        int   offset = newOffset;
        if (relative) {
            offset = setting->getValue() + newOffset;
        }
        offset = std::clamp(offset, (int)setting->getMinValue().value(), (int)setting->getMaxValue().value());
        setting->setValue(offset);

        m_fields->m_shiftOffsetInput->setString(std::to_string(offset));
        updateButtonTags(offset);

        return offset;
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

    static inline bool
    isBetterMenuRunning() {
        auto betterEdit = Loader::get()->getLoadedMod("hjfod.betteredit");
        if (betterEdit == nullptr) return false;

        return betterEdit->getSettingValue<bool>("new-color-menu");
    }
};
