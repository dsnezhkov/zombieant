// go build -o shim.so -buildmode=c-shared shim.go
// https://medium.com/learning-the-go-programming-language/calling-go-functions-from-other-languages-4c7d8bcc69bf
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

