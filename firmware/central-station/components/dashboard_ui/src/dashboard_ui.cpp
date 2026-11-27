/**
 * @file dashboard_ui.cpp
 * @brief Třída Dashboard: životní cyklus displeje a překreslování.
 */
#include "dashboard_ui.hpp"

#include <memory>
#include <new>

#include "esp_log.h"

#include "dashboard_demo.hpp"
#include "dashboard_render.hpp"

namespace dash {
namespace {

constexpr char TAG[] = "dashboard";

/** Instance, která právě vlastní displej (ovladač je jen jeden). */
const Dashboard *s_owner = nullptr;

}  // namespace

esp_err_t Dashboard::begin(const epd_config_t &config)
{
    if (canvas_) return ESP_OK;
    if (s_owner) return ESP_ERR_INVALID_STATE;

    const esp_err_t err = epd_init(&config);
    if (err != ESP_OK) return err;

    canvas_ = epd_canvas_create(EPD_ROT_0);
    if (!canvas_) {
        epd_deinit();
        return ESP_ERR_NO_MEM;
    }
    s_owner = this;
    return ESP_OK;
}

void Dashboard::end()
{
    if (!canvas_) return;

    epd_canvas_free(canvas_);
    canvas_ = nullptr;
    epd_deinit();
    epd_prepare_deep_sleep();
    s_owner = nullptr;
}

esp_err_t Dashboard::refresh()
{
    if (!canvas_) return ESP_ERR_INVALID_STATE;

    render(*canvas_, data);
    return show();
}

esp_err_t Dashboard::test(TestScreen screen)
{
    if (!canvas_) return ESP_ERR_INVALID_STATE;
    if (screen == TestScreen::LowBattery) return showLowBattery(3.31f);

    /* testovací data jdou na haldu, na zásobník jsou moc velká */
    const std::unique_ptr<DashboardData> demo(new (std::nothrow) DashboardData);
    if (!demo) return ESP_ERR_NO_MEM;

    fillDemoData(*demo);
    if (screen == TestScreen::ErrorStates) applyErrorStates(*demo);

    render(*canvas_, *demo);
    return show();
}

esp_err_t Dashboard::showLowBattery(float voltageV)
{
    if (!canvas_) return ESP_ERR_INVALID_STATE;

    renderLowBattery(*canvas_, voltageV);
    return show();
}

esp_err_t Dashboard::show()
{
    const esp_err_t err = epd_show(canvas_->fb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "překreslení selhalo: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "překresleno za %u ms", static_cast<unsigned>(epd_last_refresh_ms()));
    return ESP_OK;
}

}  // namespace dash
