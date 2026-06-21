import time
import machine
from machine import Pin, ADC, PWM, I2C

# Note: The BH1750 library must be uploaded to your MicroPython device separately.
# If you don't have it, a minimal manual I2C driver implementation is included below.
class BH1750:
    def __init__(self, i2c, addr=0x23):
        self.i2c = i2c
        self.addr = addr
        # Set to continuous high resolution mode (0x10)
        try:
            self.i2c.writeto(self.addr, b'\x10')
        except Exception:
            pass

    def read_light_level(self):
        try:
            data = self.i2c.readfrom(self.addr, 2)
            lux = ((data[0] << 8) | data[1]) / 1.2
            return lux
        except Exception:
            return 0.0

# ====================================================================
# 📌 PIN CONFIGURATIONS
# ====================================================================
MQ137_PIN = 34         # Ammonia Sensor (Analog Pin)
TDS_SENSOR_PIN = 35    # TDS Sensor (Analog Pin)
FLOW_PIN = 18          # Water Flow Sensor (Digital Interrupt Pin)
SDA_PIN = 21           # BH1750 I2C Data
SCL_PIN = 22           # BH1750 I2C Clock

RELAY_PIN = 26         # Ammonia Relay (Low-Level Trigger)
TDS_RELAY_PIN = 25     # TDS Relay (Low-Level Trigger)
LED_PIN = 23           # Light Dimming LED (PWM Pin)

# ====================================================================
# 📌 SYSTEM THRESHOLDS & SETTINGS
# ====================================================================
AMMONIA_THRESHOLD = 1000 
TDS_THRESHOLD = 40 
DARK_THRESHOLD = 1.5       
MEDIUM_THRESHOLD = 500.0   
BRIGHT_THRESHOLD = 1000.0  

PWM_FREQ = 5000              
BRIGHTNESS_OFF = 0           
BRIGHTNESS_MEDIUM = 76       
BRIGHTNESS_HIGH = 255        

VREF = 3.3            
SCOUNT = 30

# ====================================================================
# 📌 HARDWARE INITIALIZATION
# ====================================================================
# Configure Analog Inputs (12-bit Resolution: 0-4095)
mq137_adc = ADC(Pin(MQ137_PIN))
mq137_adc.atten(ADC.ATTN_11DB)  # Configures 0-3.6V range for 12-bit ADC

tds_adc = ADC(Pin(TDS_SENSOR_PIN))
tds_adc.atten(ADC.ATTN_11DB)

# Configure Digital Outputs (Relays Default to HIGH / OFF at startup)
relay_ammonia = Pin(RELAY_PIN, Pin.OUT, value=1)
relay_tds = Pin(TDS_RELAY_PIN, Pin.OUT, value=1)

# Configure hardware PWM for Dimming LED (8-bit depth emulation)
led_pwm = PWM(Pin(LED_PIN), freq=PWM_FREQ)
# MicroPython PWM uses a 10-bit range (0-1023). We'll convert 0-255 inputs to match.
def set_led_brightness(val):
    duty_10bit = int((val / 255) * 1023)
    led_pwm.duty(duty_10bit)

set_led_brightness(BRIGHTNESS_OFF)

# Configure I2C Bus Communication and Light Sensor
i2c_bus = I2C(0, scl=Pin(SCL_PIN), sda=Pin(SDA_PIN), freq=100000)
light_meter = BH1750(i2c_bus)

# ====================================================================
# 📌 GLOBAL SYSTEM VARIABLES & BUFFERS
# ====================================================================
pulse_count = 0
flow_rate = 0.0
last_saved_pulses = 0 

analog_buffer = [0] * SCOUNT
analog_buffer_index = 0
average_voltage = 0.0
tds_value = 0.0
temperature = 25.0    

current_brightness = 0
target_brightness = 0
current_lux = 0.0 

global_raw_ammonia = 0
global_ammonia_volt = 0.0

# ====================================================================
# 📌 HARDWARE INTERRUPT SERVICE ROUTINES
# ====================================================================
def count_pulse(pin):
    global pulse_count
    pulse_count += 1

# Bind Flow Sensor Interrupt
flow_sensor_pin = Pin(FLOW_PIN, Pin.IN, Pin.PULL_UP)
flow_sensor_pin.irq(trigger=Pin.IRQ_FALLING, handler=count_pulse)

# ====================================================================
# 📌 MATHEMATICAL FILTER CHANNELS
# ====================================================================
def get_median_num(b_array):
    sorted_array = sorted(b_array)
    length = len(sorted_array)
    if length % 2 == 1:
        return sorted_array[length // 2]
    else:
        return (sorted_array[length // 2] + sorted_array[(length // 2) - 1]) // 2

# ====================================================================
# 📌 TIMING REGISTRIES AND MAIN MONITOR LOOP
# ====================================================================
# 📌 FIXED: Changed \e to \x1b so Python correctly clears the terminal screen at startup
print("\x1b[2J\x1b[H", end="")

# Seed non-blocking timing execution variables using millisecond clocks
start_time = time.ticks_ms()
previous_flow_time = start_time
analog_sample_timepoint = start_time
read_delay_light_time = start_time
dashboard_update_time = start_time

while True:
    current_millis = time.ticks_ms()

    # ------------------------------------------------------------------
    # 📋 BACKGROUND DATA ACQUISITION TRACKS (Non-Blocking)
    # ------------------------------------------------------------------
    
    # Continuous Background Ammonia Reading
    global_raw_ammonia = mq137_adc.read()
    global_ammonia_volt = (global_raw_ammonia / 4095.0) * VREF

    # Background TDS Window Array Buffering (Every 40ms)
    if time.ticks_diff(current_millis, analog_sample_timepoint) >= 40:
        analog_sample_timepoint = current_millis
        analog_buffer[analog_buffer_index] = tds_adc.read()
        analog_buffer_index += 1
        if analog_buffer_index >= SCOUNT:
            analog_buffer_index = 0

    # Background Flow Count Calculation Processor (Every 1000ms)
    if time.ticks_diff(current_millis, previous_flow_time) >= 1000:
        previous_flow_time = current_millis
        flow_rate = pulse_count / 7.5
        last_saved_pulses = pulse_count
        pulse_count = 0 

    # Background BH1750 Light Intensity Processing Engine (Every 100ms)
    if time.ticks_diff(current_millis, read_delay_light_time) >= 100:
        read_delay_light_time = current_millis
        current_lux = light_meter.read_light_level()

        if current_lux >= BRIGHT_THRESHOLD:
            target_brightness = BRIGHTNESS_OFF
        elif current_lux <= DARK_THRESHOLD:
            target_brightness = BRIGHTNESS_HIGH
        else:
            target_brightness = BRIGHTNESS_MEDIUM

        # Smooth Dimming Adjustments
        if current_brightness < target_brightness:
            current_brightness += 1
            set_led_brightness(current_brightness)
        elif current_brightness > target_brightness:
            current_brightness -= 1
            set_led_brightness(current_brightness)

    # ====================================================================
    # 🖨️ CONSOLIDATED DASHBOARD PRINT ENGINE (Every 3000ms)
    # ====================================================================
    if time.ticks_diff(current_millis, dashboard_update_time) >= 3000:
        dashboard_update_time = current_millis

        # --- Math Pipeline Executions for TDS ---
        analog_buffer_temp = list(analog_buffer)
        average_voltage = get_median_num(analog_buffer_temp) * VREF / 4095.0
        compensation_coefficient = 1.0 + 0.02 * (temperature - 25.0)
        compensation_voltage = average_voltage / compensation_coefficient
        
        tds_value = (133.42 * compensation_voltage * compensation_voltage * compensation_voltage
                    - 255.86 * compensation_voltage * compensation_voltage
                    + 857.39 * compensation_voltage) * 0.5
        if tds_value < 0:
            tds_value = 0.0

        # --- Actuator Logic States Determination ---
        ammonia_relay_active = (global_raw_ammonia > AMMONIA_THRESHOLD)
        tds_relay_active = (tds_value >= TDS_THRESHOLD)

        # Drive Low-Level Relays (0 = ON, 1 = OFF)
        relay_ammonia.value(0 if ammonia_relay_active else 1)
        relay_tds.value(0 if tds_relay_active else 1)

        # --- ANSI Escaped Terminal Dashboard Layout Frame ---
        # 📌 FIXED: Changed \e to \x1b so the cursor accurately snaps back to the top left corner
        print("\x1b[H", end="") 
        print("=================================================================")
        print("                    CRAYLIFE INTEGRATED SYSTEM                   ")
        print("=================================================================")
        
        # Module Section 1: Ammonia Monitoring System
        print("[AMMONIA SYSTEM]  ADC Raw: {}\tVoltage: {:.2f}V\tStatus: ".format(global_raw_ammonia, global_ammonia_volt), end="")
        if ammonia_relay_active:
            print("🚨 CRITICAL! UV ON ")
        else:
            print("✅ SAFE. UV OFF   ")

        # Module Section 2: Total Dissolved Solids Quality System
        print("[TDS WATER MGMT]  Density: {:.0f} ppm\tVoltage: {:.2f}V\tStatus: ".format(tds_value, average_voltage), end="")
        if tds_relay_active:
            print("🚨 HIGH SOLID! PUMP ON")
        else:
            print("✅ NORMAL. PUMP OFF  ")

        # Module Section 3: Water Flow Verification Pipeline
        print("[FLOW MONITOR  ]  Pulses: {}\tVelocity: {:.1f}L/min\tStatus: ".format(last_saved_pulses, flow_rate), end="")
        if last_saved_pulses > 0:
            print("✅ WATER FLOWING      ")
        else:
            print("❌ STATIC RISK NO FLOW")

        # Module Section 4: Photonic Lighting Tracker Calibration 
        print("[LIGHT CONTROLLER] Intensity: {:.1f} lx\tPWM Output: {}\tProfile: ".format(current_lux, current_brightness), end="")
        if current_lux >= BRIGHT_THRESHOLD:
            print("☀️ BRIGHT (OFF)    ")
        elif current_lux <= DARK_THRESHOLD:
            print("🌙 DARK (HIGH)     ")
        else:
            print("⛅ MEDIUM (30%)    ")

        print("=================================================================")
    
    # Tiny sleep window to keep the CPU from burning cycles unnecessarily
    time.sleep_ms(1)