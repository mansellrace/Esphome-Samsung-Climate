#include "esphome.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "ir_Samsung.h"
#include "IRrecv.h"
#include "IRutils.h"
#include "IRac.h"
#include "IRtext.cpp"

const uint16_t kIrLed = D7; // LED PIN TX
const uint16_t kRecvPin = D6;
const float const_min = 16;
const float const_max = 30;
const bool inverted = false;
const bool use_modulation = false;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint8_t kTolerancePercentage = 40;
const uint16_t kMinUnknownSize = 12;
static const std::string CUSTOM_FAN_LEVEL_QUIET = "Quiet";
static const std::string CUSTOM_FAN_LEVEL_1 = "Velocità 1";
static const std::string CUSTOM_FAN_LEVEL_2 = "Velocità 2";
static const std::string CUSTOM_FAN_LEVEL_3 = "Velocità 3";
static const std::string CUSTOM_FAN_LEVEL_4 = "Velocità 4";


IRSamsungAc ac(kIrLed, inverted, use_modulation);
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);

decode_results results;

class SamsungAC : public Component, public Climate {
  public:
    sensor::Sensor *sensor_{nullptr};
    template_::TemplateSwitch *fast_{nullptr};

    void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
    void set_fast(template_::TemplateSwitch *fast) { this->fast_ = fast; }


    void setup() override
    {
      irrecv.setUnknownThreshold(kMinUnknownSize);
      irrecv.setTolerance(kTolerancePercentage);
      irrecv.enableIRIn();

      if (this->sensor_) {
        this->sensor_->add_on_state_callback([this](float state) {
          this->current_temperature = state;
          this->publish_state();
        });
        this->current_temperature = this->sensor_->state;
      } else {
        this->current_temperature = NAN;
      }

      if (this->fast_) {
        this->fast_->add_on_state_callback([this](bool state) {
          ESP_LOGD("DEBUG", "Fast  %s", state ? "ON" : "OFF");
          if (state) {
            ac.setPowerful(true);
            irrecv.disableIRIn();
            ac.send();
            irrecv.enableIRIn();
          } else {
            ac.setPowerful(false);
          }
        });
      }

      auto restore = this->restore_state_();
      if (restore.has_value()) {
        restore->apply(this);
      } else {
        this->mode = climate::CLIMATE_MODE_OFF;
        this->target_temperature = roundf(clamp(this->current_temperature, const_min, const_max));
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
        this->swing_mode = climate::CLIMATE_SWING_OFF;
      }

      if (isnan(this->target_temperature)) {
        this->target_temperature = 25;
      }

      ac.begin();
      ac.on();

      if (this->mode == CLIMATE_MODE_OFF) {
        ac.off();
      } else if (this->mode == CLIMATE_MODE_COOL) {
        ac.setMode(kSamsungAcCool);
      } else if (this->mode == CLIMATE_MODE_DRY) {
        ac.setMode(kSamsungAcDry);
      } else if (this->mode == CLIMATE_MODE_FAN_ONLY) {
        ac.setMode(kSamsungAcFan);
      } else if (this->mode == CLIMATE_MODE_HEAT) {
        ac.setMode(kSamsungAcHeat);
      } else if (this->mode == CLIMATE_MODE_HEAT_COOL) {
        ac.setMode(kSamsungAcFanAuto);
      } 
      ac.setTemp(this->target_temperature);
      if (this->fan_mode == CLIMATE_FAN_AUTO) {
        ac.setFan(kSamsungAcFanAuto);
      } else if (this->fan_mode == CLIMATE_FAN_LOW) {
        ac.setFan(kSamsungAcFanLow);
      } else if (this->fan_mode == CLIMATE_FAN_MEDIUM) {
        ac.setFan(kSamsungAcFanMed);
      } else if (this->fan_mode == CLIMATE_FAN_HIGH) {
        ac.setFan(kSamsungAcFanHigh);
      }
      if (this->swing_mode == CLIMATE_SWING_OFF) {
        ac.setSwing(false);
      } else if (this->swing_mode == CLIMATE_SWING_VERTICAL) {
        ac.setSwing(true);
      }

      irrecv.disableIRIn();

      if (this->mode == CLIMATE_MODE_OFF) {
        ac.sendOff();
      } else {
        ac.send();
      }
      this->publish_state();

      irrecv.enableIRIn();

      ESP_LOGD("DEBUG", "Samsung A/C remote is in the following state:");
      ESP_LOGD("DEBUG", "  %s\n", ac.toString().c_str());
    }

    void loop() override
    {
      if (irrecv.decode(&results)) {
        ESP_LOGD("IR_ricevuto", "%s", resultToHumanReadableBasic(&results).c_str());
        String description = IRAcUtils::resultAcToString(&results);
        if (description.length()) {
          ESP_LOGD("IR_ricevuto", "  %s\n", description.c_str());

          ac.setRaw(results.state, results.bits / 8);

          bool power = ac.getPower();
          if (power == false) {
            this->mode = CLIMATE_MODE_OFF;
          } else {
            id(mod_fast).publish_state(false);
            switch (ac.getMode()) {
              case kSamsungAcCool:
                  this->mode = CLIMATE_MODE_COOL;
                  break;
              case kSamsungAcDry:
                  this->mode = CLIMATE_MODE_DRY;
                  break;
              case kSamsungAcFan:
                  this->mode = CLIMATE_MODE_FAN_ONLY;
                  break;
              case kSamsungAcHeat:
                  this->mode = CLIMATE_MODE_HEAT;
                  break;
              case kSamsungAcAuto:
                  this->mode = CLIMATE_MODE_HEAT_COOL;
            }
          }

          if (power) {
            if (ac.getQuiet()) {
              this->set_custom_fan_mode_(CUSTOM_FAN_LEVEL_QUIET);
            } else {
              switch (ac.getFan()) {
                case kSamsungAcFanAuto:
                  this->set_fan_mode_(CLIMATE_FAN_AUTO);
                  break;
                case kSamsungAcFanLow:
                  this->set_custom_fan_mode_(CUSTOM_FAN_LEVEL_1);
                  break;
                case kSamsungAcFanMed:
                  this->set_custom_fan_mode_(CUSTOM_FAN_LEVEL_2);
                  break;
                case kSamsungAcFanHigh:
                  this->set_custom_fan_mode_(CUSTOM_FAN_LEVEL_3);
                  break;
                case kSamsungAcFanTurbo:
                  this->set_custom_fan_mode_(CUSTOM_FAN_LEVEL_4);
              }
            }

            switch (ac.getSwing()) {
              case false:
                this->swing_mode = CLIMATE_SWING_OFF;
                break;
              case true:
                this->swing_mode = CLIMATE_SWING_VERTICAL;
            }

            this->target_temperature = ac.getTemp();
          }


          this->publish_state();
        }
        irrecv.resume();
      }
    }

    climate::ClimateTraits traits() {
      auto traits = climate::ClimateTraits();
      traits.set_visual_min_temperature(const_min);
      traits.set_visual_max_temperature(const_max);
      traits.set_visual_temperature_step(1);
      traits.set_supported_modes({
          climate::CLIMATE_MODE_OFF,
          climate::CLIMATE_MODE_HEAT_COOL,
          climate::CLIMATE_MODE_COOL,
          climate::CLIMATE_MODE_DRY,
          climate::CLIMATE_MODE_HEAT,
          climate::CLIMATE_MODE_FAN_ONLY,
      });
      traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_QUIET);
      traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_1);
      traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_2);
      traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_3);
      traits.add_supported_custom_fan_mode(CUSTOM_FAN_LEVEL_4);
      traits.add_supported_fan_mode(CLIMATE_FAN_AUTO);
      traits.set_supported_swing_modes({
          climate::CLIMATE_SWING_OFF,
          climate::CLIMATE_SWING_VERTICAL,
      });
      traits.set_supports_current_temperature(true);
      return traits;
    }

  void control(const ClimateCall &call) override {
    bool fan_control = true;
    id(mod_fast).publish_state(false);
    if (call.get_mode().has_value()) {
      ClimateMode mode = *call.get_mode();
      if (mode == CLIMATE_MODE_OFF) {
        ac.off();
      } else if (mode == CLIMATE_MODE_COOL) {
        ac.on();
        ac.setMode(kSamsungAcCool);
      } else if (mode == CLIMATE_MODE_FAN_ONLY) {
        ac.on();
        ac.setMode(kSamsungAcFan);
      } else if (mode == CLIMATE_MODE_HEAT) {
        ac.on();
        ac.setMode(kSamsungAcHeat);
      } else if (mode == CLIMATE_MODE_DRY) {
        ac.on();
        ac.setMode(kSamsungAcDry);
      } else if (mode == CLIMATE_MODE_HEAT_COOL) {
        ac.on();
        ac.setMode(kSamsungAcAuto);
      }
      this->mode = mode;
    }

    if (call.get_target_temperature().has_value()) {
      float temp = *call.get_target_temperature();
      ac.setTemp(temp);
      this->target_temperature = temp;
    }

    if (this->mode == CLIMATE_MODE_DRY || this->mode == CLIMATE_MODE_HEAT_COOL) {
      ac.setFan(kSamsungAcFanAuto);
      this->fan_mode = CLIMATE_FAN_AUTO;
      fan_control = false;
    }

    if (fan_control) {
      if (call.get_fan_mode().has_value()) {
        ClimateFanMode fan_mode = *call.get_fan_mode();
        if (fan_mode == CLIMATE_FAN_AUTO) {
          ac.setFan(kSamsungAcFanAuto);
        }
        this->fan_mode = fan_mode;
      }

      if (call.get_custom_fan_mode().has_value()) {
        auto custom_fan_mode = *call.get_custom_fan_mode();
        if (custom_fan_mode == "Quiet") { 
          if (this->mode != CLIMATE_MODE_FAN_ONLY) {
            ac.setQuiet(true);
          } else {
            ac.setQuiet(false);
            ac.setFan(kSamsungAcFanLow);
          }
        } else { 
          ac.setQuiet(false);
          if (custom_fan_mode == "Velocità 1") {
              ac.setFan(kSamsungAcFanLow);
          } else if (custom_fan_mode == "Velocità 2") {
              ac.setFan(kSamsungAcFanMed);
          } else if (custom_fan_mode == "Velocità 3") {
              ac.setFan(kSamsungAcFanHigh);
          } else if (custom_fan_mode == "Velocità 4") {
              ac.setFan(kSamsungAcFanTurbo);
          }
        }
        this->custom_fan_mode = custom_fan_mode;
      }     
    }

    if (call.get_swing_mode().has_value()) {
      ClimateSwingMode swing_mode = *call.get_swing_mode();
      if (swing_mode == CLIMATE_SWING_OFF) {
        ac.setSwing(false);
      } else if (swing_mode == CLIMATE_SWING_VERTICAL) {
        ac.setSwing(true);
      }
      this->swing_mode = swing_mode;
    }
    irrecv.disableIRIn();
    if (this->mode == CLIMATE_MODE_OFF) {
      ac.sendOff();
    } else {
      ac.send();
    }
    this->publish_state();
    irrecv.enableIRIn();

    ESP_LOGD("DEBUG", "Samsung A/C remote is in the following state:");
    ESP_LOGD("DEBUG", "  %s\n", ac.toString().c_str());
  }
};