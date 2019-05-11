// go build -o shim.so -buildmode=c-shared shim.go
package main


import "C"

import (
	"fmt"
)

var count int

//export Entry
func Entry(msg string) int {
	fmt.Println(msg)
	return count
}

func main() {}

