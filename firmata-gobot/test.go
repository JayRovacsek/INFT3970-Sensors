package main

import (
	"C"
	"dht"
	"fmt"
	"net/http"
	"time"

	"gobot.io/x/gobot"
	"gobot.io/x/gobot/drivers/gpio"
	"gobot.io/x/gobot/platforms/firmata"
)

const (
	SensorDHT11 = iota
	SensorDHT22
)

var firmataAdaptor = firmata.NewTCPAdaptor("192.168.1.95:3030")

func main() {

	work := func() {
		gobot.Every(1*time.Second, func() {
			tryFlashLED()
		})
	}

	led := gpio.NewLedDriver(firmataAdaptor, "2")

	robot := gobot.NewRobot("bot",
		[]gobot.Connection{firmataAdaptor},
		[]gobot.Device{led},
		work,
	)

	robot.Start()
}

func tryFlashLED() {
	var available = getAvailability(`http://inft3970.azurewebsites.net:80/api/Availability`)

	fmt.Println(available)

	if available {
		flashLED(`100ms`, 10)
		return
	}

	flashLED(`500`, 2)
	return
}

func flashLED(milliseconds string, iterations int) {
	led := gpio.NewLedDriver(firmataAdaptor, "2")

	duration, _ := time.ParseDuration(milliseconds)

	for i := 1; i <= iterations; i++ {
		led.Toggle()
		time.Sleep(duration)
		led.Toggle()
	}
}

func getAvailability(server string) bool {

	response, err := http.Get(server)

	var humidity, temperature, _ = getSensorData(SensorDHT11, 2)
	if err != nil {
		printError(err)
	}

	fmt.Println(fmt.Sprintf("Temperature: %f, Humidity: %f"), temperature, humidity)

	if err != nil {
		printError(err)
	}

	if response.StatusCode == 200 {
		return true
	}
	return false
}

func getSensorData(stype, pin int) (humidity, temperature float32, err error) {
	if stype != SensorDHT11 && stype != SensorDHT22 {
		err = fmt.Errorf("sensor type must be either %d or %d", SensorDHT11, SensorDHT22)
		return
	}

	var data [5]byte
	data, err = dht.ReadSensor(pin)
	if err != nil {
		return
	}

	if stype == SensorDHT11 {
		humidity = float32(data[0])
		temperature = float32(data[2])
	} else {
		humidity = float32(int(data[0])*256+int(data[1])) / 10.0
		temperature = float32(int(data[2]&0x7F)*256+int(data[3])) / 10.0
		if data[2]&0x80 > 0 {
			temperature *= -1.0
		}
	}
	return
}

func printError(err error) {
	fmt.Println(fmt.Sprintf("An error occured: %v", err.Error()))
}
