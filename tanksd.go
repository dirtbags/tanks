package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path"
	"time"
)

var forftanksPath = flag.String("forftanks", "./forftanks", "path to forftanks executable")
var wwwDir = flag.String("www", "www", "path to www http content (ro)")
var tanksDir = flag.String("tanks", "tanks", "path to tanks state directories (rw)")
var roundsDir = flag.String("rounds", "rounds", "path to rounds storage (rw)")
var maxrounds = flag.Uint("maxrounds", 200, "number of rounds to store")
var maxSize = flag.Uint("maxsize", 8000 , "maximum uploaded file size")
var listenAddr = flag.String("listen", ":8080", "where to listen for incoming HTTP connections")
var roundDuration = flag.Duration("round", 1 * time.Minute, "Time to wait between each round")

type TankState struct {
	dir string
	roundsdir string
}

var validFilenames = []string{
	"author",
	"name",
	"color",
	"program",
	"sensor0",
	"sensor1",
	"sensor2",
	"sensor3",
	"sensor4",
	"sensor5",
	"sensor6",
	"sensor7",
	"sensor8",
	"sensor9",
}
func (ts *TankState) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	id := req.PathValue("id")
	name := req.PathValue("name")

	if req.ContentLength < 0 {
		http.Error(w, "Length required", http.StatusLengthRequired)
		return
	}
	if uint(req.ContentLength) > *maxSize {
		http.Error(w, "Too large", http.StatusRequestEntityTooLarge)
		return
	}

	tankDir := path.Join(ts.dir, id)
	if tankDir == ts.dir {
		http.Error(w, "Invalid tank ID", http.StatusBadRequest)
		return
	}

	filename := path.Join(tankDir, name)
	f, err := os.Create(filename)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	defer f.Close()
	if _, err := io.Copy(f, req.Body); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	fmt.Fprintln(w, "file written")
}

func (ts *TankState) WriteRound(now time.Time, round []byte) error {
	dents, err := os.ReadDir(ts.roundsdir)
	if err != nil {
		return err
	}

	for uint(len(dents)) > *maxrounds {
		fn := path.Join(ts.roundsdir, dents[0].Name())
		if err := os.Remove(fn); err != nil {
			return err
		}
		dents = dents[1:]
	}

	roundFn := fmt.Sprintf("%016x.json", now.Unix())
	roundPath := path.Join(ts.roundsdir, roundFn)
	if err := os.WriteFile(roundPath, round, 0644); err != nil {
		return err
	}

	rounds := make([]string, len(dents) + 1)
	for i := 0; i < len(dents); i++ {
		rounds[i] = dents[i].Name()
	}
	rounds[len(dents)] = roundFn

	roundsJs, err := json.Marshal(rounds)
	if err != nil {
		return err
	}
	idxFn := path.Join(ts.roundsdir, "index.json")
	if err := os.WriteFile(idxFn, roundsJs, 0644); err != nil {
		return err
	}

	return nil
}

func (ts *TankState) RunRound(now time.Time) error {
	dents, err := os.ReadDir(ts.dir)
	if err != nil {
		return err
	}

	args := make([]string, 0, len(dents))
	for _, dent := range dents {
		if dent.IsDir() {
			tankPath := path.Join(ts.dir, dent.Name())
			args = append(args, tankPath)
		}
	}

	if len(args) < 2 {
		return fmt.Errorf("Not enough tanks for a round")
	}

	cmd := exec.Command(*forftanksPath, args...)
	out, err := cmd.Output()
	if err != nil {
		return err
	}

	if err := ts.WriteRound(now, out); err != nil {
		return err
	}
	
	return nil
}

func (ts *TankState) RunForever() {
	if err := ts.RunRound(time.Now()); err != nil {
		log.Println(err)
	}

	for now := range time.Tick(*roundDuration) {
		if err := ts.RunRound(now); err != nil {
			log.Println(err)
		}
	}
}

func main() {
	flag.Parse()

	ts := &TankState{
		dir: *tanksDir,
		roundsdir: *roundsDir,
	}

	http.Handle("GET /", http.FileServer(http.Dir(*wwwDir)))
	http.Handle("GET /rounds/", http.StripPrefix("/rounds/", http.FileServer(http.Dir(*roundsDir))))
	http.Handle("PUT /tanks/{id}/{name}", ts)

	go ts.RunForever()

	log.Println("Listening on", *listenAddr)
	http.ListenAndServe(*listenAddr, nil)
}
