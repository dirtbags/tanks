package main

import (
	"fmt"
	"math/rand"
	"strconv"
	"time"
	"strings"
	"io/ioutil"
	"encoding/json"
)

type sensor struct {
	srange int
	angle int
	width int
	turret bool
}

type Tank struct {
	dna []int
	color string
	sensors [10]sensor
	program []string
}

var symbols []string = []string{
	"~",
  "!",
  "+",
  "-",
  "*",
  "/",
  "%",
  "&",
  "|",
  "^",
  "<<",
  ">>",
  ">",
  ">=",
  "<",
  "<=",
  "=",
  "<>",
  "abs",
  "dup",
  "pop",
  "exch",
  "if",
  "ifelse",
  "mset",
  "mget",
  "{",
  "}",
  "fire-ready?",
  "fire!",
  "set-speed!",
  "get-turret",
  "set-turret!",
  "sensor?",
  "set-led!",
  "random",
}

func make_nucleotide() int {
	switch rand.Intn(3) {
	case 0:
	  return rand.Intn(len(symbols))
	case 1:
	  return len(symbols) + rand.Intn(720)
	 case 2:
		return len(symbols) + 720 + rand.Intn(100)
	}
	return 0
}

func make_dna(size int) []int {
	ret := make([]int, size)
	for i := 0; i < size; i += 1 {
		ret[i] = make_nucleotide()
	}
	return ret
}

func color_of_dna(dna []int) (string, []int) {
	if len(dna) >= 3 {
		r := dna[0] % 256
		g := dna[1] % 256
		b := dna[2] % 256
		color := fmt.Sprintf("%02x%02x%02x", r, g, b)
		return color, dna[3:]
	} else {
		return "cccccc", dna
	}
}

func sensor_of_dna(dna []int) (sensor, []int) {
	if len(dna) >= 4 {
		return sensor{dna[0], dna[1], dna[2], dna[3] % 2 == 0}, dna[4:]
	} else {
		return sensor{0, 0, 0, false}, dna
	}
}

func program_of_dna(dna []int) []string {
	ret := make([]string, 0, len(dna))
	
	stacks := []int{}
	for i := 0; i < len(dna); i += 1 {
		// If we are in a stack, decrement the size
		if len(stacks) > 0 {
			stacks[0] -= 1
			// Are we at the end of the stack? Output the symbol and pop the stack off.
			if stacks[0] <= 0 {
				ret = append(ret, "}")
				stacks = stacks[1:]
			}
		}
		
		// Consider the next nucleotide
		n := dna[i]

		// Symbol?
		if n < len(symbols) {
			ret = append(ret, symbols[n])
			continue
		}
		n -= len(symbols)
		
		// Number?
		if n < 720 {
			ret = append(ret, strconv.Itoa(n - 360))
			continue
		}
		n -= 720
		
		// Stack.
		// XXX: This doesn't seem right, we shouldn't see a ton of "} } }" at the end
		ret = append(ret, "{")
		stacks = append(stacks, (n + 1) % (len(dna) - i))
	}
	
	for len(stacks) > 0 {
		ret = append(ret, "}")
		stacks = stacks[1:]
	}
	
	return ret
}

func tank_of_dna(dna []int) Tank {
	ret := Tank{dna: dna}
	ret.color, dna = color_of_dna(dna)
	for i := 0; i < len(ret.sensors); i += 1 {
		ret.sensors[i], dna = sensor_of_dna(dna)
	}
	ret.program = program_of_dna(dna)
	
	return ret
}

func twrite(num int, fn string, contents string) {
	path := fmt.Sprintf("tank.%d/%s", num, fn)
	ioutil.WriteFile(path, []byte(contents), 0644)
}

func (t Tank) Write(num int, dir string) {
	twrite(num, "color", t.color)
	for i := 0; i < len(t.sensors); i += 1 {
		turret := 0
		if t.sensors[i].turret {
			turret = 1
		}
		twrite(
			num,
			fmt.Sprintf("sensor%d", i),
			fmt.Sprintf("%d %d %d %d", t.sensors[i].srange, t.sensors[i].angle, t.sensors[i].width, turret),
		)
	}
	twrite(num, "program", strings.Join(t.program, " "))
	
	s, _ := json.Marshal(t.dna)
	twrite(num, "dna", string(s))
}

func init() {
	rand.Seed(time.Now().UTC().UnixNano())
}

func main() {
	d := make_dna(200)
	t := tank_of_dna(d)
	t.Write(0, ".")
}