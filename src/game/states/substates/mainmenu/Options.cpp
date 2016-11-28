#include "Options.h"

#include "game/AppContext.h"
#include "game/layout/options/OptionsItem.h"
#include "game/layout/options/ToggleBtn.h"
#include "game/layout/options/ValueChooser.h"
#include "game/states/MainMenuState.h"
#include "game/util/CircularModulo.h"
#include "system/GraphicsContext.h"
#include "system/Localize.h"
#include "system/Texture.h"

#include <assert.h>
#include <algorithm>


namespace SubStates {
namespace MainMenu {

Options::Options(MainMenuState& parent, AppContext& app)
    : current_category_idx(0)
    , current_setting_idx(0)
{
    category_buttons.emplace_back(app, tr("GENERAL"));
    category_buttons.emplace_back(app, tr("FINE TUNING"));
    category_buttons.emplace_back(app, tr("INPUT"));
    category_buttons.at(current_category_idx).onHoverEnter();

    using ToggleButton = Layout::Options::ToggleButton;
    using ValueChooser = Layout::Options::ValueChooser;

    std::vector<std::unique_ptr<Layout::Options::OptionsItem>> system_options;
    system_options.emplace_back(std::make_unique<ToggleButton>(
        app, false, tr("Fullscreen mode"),
        tr("Toggle fullscreen mode. On certain (embedded) devices, this setting may have no effect."),
        [&app](bool){
            app.window().toggleFullscreen();
        }));
    system_options.back()->setMarginBottom(40);
    system_options.emplace_back(std::make_unique<ToggleButton>(app, true, tr("Sound effects")));
    system_options.emplace_back(std::make_unique<ToggleButton>(app, true, tr("Background music")));
    subitem_panels.push_back(std::move(system_options));

    std::vector<std::unique_ptr<Layout::Options::OptionsItem>> tuning_options;
    {
        tuning_options.emplace_back(std::make_unique<ValueChooser>(app,
            std::vector<std::string>({tr("SRS"), tr("TGM"), tr("Classic")}), 0, tr("Rotation style"),
            std::string(tr("SRS: The rotation style used by most commercial falling block games.\n")) +
            tr("TGM: A popular style common in far eastern games and arcade machines.\n") +
            tr("Classic: The rotation style of old console games; it does not allow wall kicking.")));
        std::vector<std::string> das_values(20);

        int k = 0;
        std::generate(das_values.begin(), das_values.end(), [&k]{ return std::to_string(++k) + "/60 s"; });
        auto das_repeat_values = das_values;
        tuning_options.emplace_back(std::make_unique<ValueChooser>(app,
            std::move(das_values), 13, tr("DAS initial delay"),
            tr("The time it takes to turn on horizontal movement autorepeat.")));
        tuning_options.emplace_back(std::make_unique<ValueChooser>(app,
            std::move(das_repeat_values), 3, tr("DAS repeat delay"),
            tr("Horizontal movement delay during autorepeat.")));

        tuning_options.emplace_back(std::make_unique<ToggleButton>(app, false, tr("Sonic drop"),
            tr("If set to 'ON', hard drop does not lock the piece instantly.")));
        tuning_options.emplace_back(std::make_unique<ValueChooser>(app,
            std::vector<std::string>({tr("Instant"), tr("Extended"), tr("Infinite")}), 1, tr("Piece lock style"),
            std::string(tr("Instant: The piece locks instantly when it reaches the ground.\n")) +
            tr("Extended: You can move or rotate the piece 10 times before it locks.\n") +
            tr("Infinite: You can move or rotate the piece an infinite number of times.")));

        k = 0;
        std::vector<std::string> lockdelay_values(60);
        std::generate(lockdelay_values.begin(), lockdelay_values.end(), [&k]{ return std::to_string(++k) + "/60 s"; });
        tuning_options.emplace_back(std::make_unique<ValueChooser>(app,
            std::move(lockdelay_values), 29, tr("Lock delay"),
            tr("The time while you can still move the piece after it reaches the ground. See 'Piece lock style'.")));

        tuning_options.emplace_back(std::make_unique<ToggleButton>(app, true, tr("Enable T-Spins"),
            tr("Allow T-Spin detection and scoring. Works best with SRS rotation.")));
        tuning_options.emplace_back(std::make_unique<ToggleButton>(app, true, tr("Detect T-Spins at the walls"),
            tr("Allow detecting T-Spins even when a corner of the T-Slot is outside of the Well.\nRequires the 'Enable T-Spins' option.")));
        tuning_options.emplace_back(std::make_unique<ToggleButton>(app, true, tr("Allow T-Spins by wallkick"),
            tr("Allow detecting T-Spins created by wall kicking.\nRequires the 'Enable T-Spins' option.")));
    }
    subitem_panels.push_back(std::move(tuning_options));


    updatePositions(app.gcx());

    fn_category_input = [this, &parent](InputType input){
        switch (input) {
            case InputType::MENU_OK:
                current_input_handler = &fn_settings_input;
                subitem_panels.at(current_category_idx).at(current_setting_idx)->onHoverEnter();
                break;
            case InputType::MENU_CANCEL:
                assert(parent.states.size() > 1);
                parent.states.pop_back();
                break;
            case InputType::MENU_UP:
                category_buttons.at(current_category_idx).onHoverLeave();
                current_category_idx = circularModulo(
                    static_cast<int>(current_category_idx) - 1, category_buttons.size());
                category_buttons.at(current_category_idx).onHoverEnter();
                break;
            case InputType::MENU_DOWN:
                category_buttons.at(current_category_idx).onHoverLeave();
                current_category_idx = circularModulo(
                    static_cast<int>(current_category_idx) + 1, category_buttons.size());
                category_buttons.at(current_category_idx).onHoverEnter();
                break;
            default:
                break;
        }
    };
    fn_settings_input = [this](InputType input){
        auto& panel = subitem_panels.at(current_category_idx);
        switch (input) {
            case InputType::MENU_CANCEL:
                panel.at(current_setting_idx)->onHoverLeave();
                current_setting_idx = 0;
                current_input_handler = &fn_category_input;
                break;
            case InputType::MENU_UP:
                panel.at(current_setting_idx)->onHoverLeave();
                current_setting_idx = circularModulo(
                    static_cast<int>(current_setting_idx) - 1, panel.size());
                panel.at(current_setting_idx)->onHoverEnter();
                break;
            case InputType::MENU_DOWN:
                panel.at(current_setting_idx)->onHoverLeave();
                current_setting_idx = circularModulo(
                    static_cast<int>(current_setting_idx) + 1, panel.size());
                panel.at(current_setting_idx)->onHoverEnter();
                break;
            case InputType::MENU_LEFT:
                panel.at(current_setting_idx)->onLeftPress();
                break;
            case InputType::MENU_RIGHT:
                panel.at(current_setting_idx)->onRightPress();
                break;
            default:
                break;
        }
    };
    current_input_handler = &fn_category_input;
}

Options::~Options() = default;

void Options::updatePositions(GraphicsContext& gcx)
{
    const int width = gcx.screenWidth() * 0.90;
    const int height = 600; // TODO: fix magic numbers
    container_rect = {
        (gcx.screenWidth() - width) / 2, (gcx.screenHeight() - height) / 2,
        width, height
    };
    screen_rect = {0, 0, gcx.screenWidth(), gcx.screenHeight()};

    const int inner_x = container_rect.x + 12;
    category_buttons.at(0).setPosition(inner_x, container_rect.y + 12);
    for (unsigned i = 1; i < category_buttons.size(); i++) {
        const auto& prev = category_buttons.at(i - 1);
        category_buttons.at(i).setPosition(inner_x, prev.y() + prev.height() + 6);
    }

    const int category_column_width = category_buttons.at(0).width() + 12;
    const int subpanel_x = container_rect.x + category_column_width + 30;
    const int subpanel_item_width = container_rect.w - category_column_width - 30 * 2;
    for (auto& subpanel : subitem_panels) {
        subpanel.at(0)->setPosition(subpanel_x, container_rect.y + 30);
        subpanel.at(0)->setWidth(subpanel_item_width);
        for (unsigned i = 1; i < subpanel.size(); i++) {
            const auto& prev = subpanel.at(i - 1);
            subpanel.at(i)->setPosition(subpanel_x, prev->y() + prev->height() + prev->marginBottom());
            subpanel.at(i)->setWidth(subpanel_item_width);
        }
    }
}

void Options::update(MainMenuState& parent, const std::vector<Event>& events, AppContext& app)
{
    parent.states.front()->updateAnimationsOnly(parent, app);

    for (const auto& event : events) {
        switch (event.type) {
            case EventType::INPUT:
                if (!event.input.down())
                    continue;
                (*current_input_handler)(event.input.type());
                break;
            default:
                break;
        }
    }
}

void Options::draw(MainMenuState& parent, GraphicsContext& gcx) const
{
    parent.states.front()->draw(parent, gcx);

    RGBAColor black = 0x00000080_rgba;
    gcx.drawFilledRect(screen_rect, black);
    RGBAColor panel_bg = 0x002060F0_rgba;
    gcx.drawFilledRect(container_rect, panel_bg);

    for (const auto& btn : category_buttons)
        btn.draw(gcx);

    assert(current_category_idx < subitem_panels.size());
    for (const auto& btn : subitem_panels.at(current_category_idx))
        btn->draw(gcx);

    if (current_input_handler == &fn_settings_input) {
        auto& description_tex = subitem_panels.at(current_category_idx).at(current_setting_idx)->descriptionTex();
        auto bgrect = container_rect;
        bgrect.h = description_tex->height() + 10;
        bgrect.y += container_rect.h - bgrect.h;
        gcx.drawFilledRect(bgrect, black);
        description_tex->drawAt(bgrect.x + 20, bgrect.y + 5);
    }
}

} // namespace MainMenu
} // namespace SubStates