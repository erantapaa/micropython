/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// qstrs specific to this port

Q(help)

// pyb module
Q(pyb)
Q(info)
Q(freq)
Q(millis)
Q(elapsed_millis)
Q(micros)
Q(elapsed_micros)
Q(delay)
Q(udelay)
Q(sync)
Q(hard_reset)
Q(unique_id)

// uos module
Q(uos)
Q(os)
Q(uname)
Q(sysname)
Q(nodename)
Q(release)
Q(version)
Q(machine)

Q(esp)
Q(socket)
Q(connect)
Q(disconnect)
Q(wifi_mode)
Q(phy_mode)
Q(sleep_type)
Q(deepsleep)
Q(adc)
Q(vdd33)
Q(chip_id)
Q(flash_id)
Q(flash_read)
Q(sdk_version)
Q(mac)
Q(getaddrinfo)
Q(send)
Q(sendto)
Q(recv)
Q(recvfrom)
Q(listen)
Q(accept)
Q(bind)
Q(settimeout)
Q(setblocking)
Q(setsockopt)
Q(close)
Q(protocol)
Q(getpeername)
Q(onconnect)
Q(onrecv)
Q(onsent)
Q(ondisconnect)
Q(onreconnect)
Q(state)
Q(MODE_11B)
Q(MODE_11G)
Q(MODE_11N)
Q(SLEEP_NONE)
Q(SLEEP_LIGHT)
Q(SLEEP_MODEM)
Q(STA_MODE)
Q(AP_MODE)
Q(STA_AP_MODE)
// os_task
Q(os_task)
Q(__init__)
Q(post)
Q(handler)
// network module
Q(network)
Q(WLAN)
Q(scan)
Q(status)
Q(isconnected)
Q(STAT_IDLE)
Q(STAT_CONNECTING)
Q(STAT_WRONG_PASSWORD)
Q(STAT_NO_AP_FOUND)
Q(STAT_CONNECT_FAIL)
Q(STAT_GOT_IP)

Q(ESPCONN_NONE)
Q(ESPCONN_WAIT)
Q(ESPCONN_CLOSE)
Q(ESPCONN_CONNECT)
Q(ESPCONN_WRITE)
Q(ESPCONN_READ)
Q(ESPCONN_CLOSE)

Q(romrun)

// Pin class
Q(Pin)
Q(init)
Q(mode)
Q(pull)
Q(value)
Q(low)
Q(high)
Q(IN)
Q(OUT_PP)
Q(OUT_OD)
Q(PULL_NONE)
Q(PULL_UP)
Q(PULL_DOWN)

// RTC
Q(RTC)
Q(datetime)
Q(memory)

// ADC
Q(ADC)
Q(read)

// utime
Q(utime)
Q(localtime)
Q(mktime)
Q(sleep)
Q(time)

// ESP os_timer
Q(os_timer)
Q(period)
Q(callback)
Q(repeat)
Q(cancel)

Q(re)
Q(json)

// mod_esp_gpio
Q(print)
// Q(toggle)
Q(gpio)
Q(INTR_DISABLE)
Q(INTR_POSEDGE)
Q(INTR_NEGEDGE)
Q(INTR_ANYEDGE)
Q(INTR_LOLEVEL)
Q(INTR_HILEVEL)

// mod_esp_dht
Q(dht)
Q(arg)
Q(pin)
Q(task)
Q(spinwait)
Q(test)
Q(consume)
Q(DATA_SIZE)

// mod_esp_mutex
Q(mutex)
Q(acquire)
Q(release)
Q(spin_time)


// esp I2C
Q(I2C)
Q(sda_pin)
Q(scl_pin)
Q(read)
Q(write)
Q(address)
Q(data)
Q(register)

// esp 1wire
Q(one_wire)
Q(reset)
Q(suppress_skip)
Q(power)
Q(edge)

Q(CONVERT_T)
Q(READ_SCRATCH_PAD)
Q(WRITE_SCRATCH_PAD)
Q(COPY_SCRATCH_PAD)
Q(RECALL_E)
Q(READ_POWER_SUPPLY)
Q(READ_ROM)
Q(MATCH_ROM)
Q(SEARCH_ROM)
Q(SKIP_ROM)

// queue
Q(queue)
Q(put)
Q(get)
Q(in_place)
Q(storage)
