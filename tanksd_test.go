package main

import (
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func testTankState(t *testing.T) (*TankState, string) {
	t.Helper()
	root := t.TempDir()
	id := "private-review-token"
	ts := &TankState{dir: filepath.Join(root, "tanks"), roundsdir: filepath.Join(root, "rounds")}
	for _, dir := range []string{filepath.Join(ts.dir, id), ts.roundsdir} {
		if err := os.MkdirAll(dir, 0700); err != nil {
			t.Fatal(err)
		}
	}
	return ts, id
}

func TestTankDownloads(t *testing.T) {
	ts, id := testTankState(t)
	program := "37 40 set-speed!\n"
	for _, name := range append(append([]string{}, validFilenames...), "private-note") {
		if err := os.WriteFile(filepath.Join(ts.dir, id, name), []byte(program), 0600); err != nil {
			t.Fatal(err)
		}
	}
	handler := ts.HTTPHandler()

	for _, name := range validFilenames {
		t.Run(name, func(t *testing.T) {
			w := httptest.NewRecorder()
			handler.ServeHTTP(w, httptest.NewRequest(http.MethodGet, "/tanks/"+id+"/"+name, nil))
			if w.Code != http.StatusOK || w.Body.String() != program {
				t.Fatalf("download: status %d, body %q", w.Code, w.Body.String())
			}
		})
	}

	for _, target := range []string{
		"/tanks/", "/tanks/%2e/", "/tanks/%2e%2e/", "/tanks/%2f/",
		"/tanks/no-such-id/%2e%2e/", "/tanks/%2e%2e/program",
		"/tanks/" + id + "/", "/tanks/" + id + "/private-note",
		"/tanks/" + id + "/%2e%2e/", "/tanks/" + id + "%2f../program",
	} {
		t.Run(target, func(t *testing.T) {
			w := httptest.NewRecorder()
			handler.ServeHTTP(w, httptest.NewRequest(http.MethodGet, target, nil))
			if w.Code != http.StatusNotFound || strings.Contains(w.Body.String(), id) {
				t.Fatalf("unexpected disclosure: status %d, body %q", w.Code, w.Body.String())
			}
		})
	}

	w := httptest.NewRecorder()
	handler.ServeHTTP(w, httptest.NewRequest(http.MethodHead, "/tanks/"+id+"/program", nil))
	if w.Code != http.StatusOK || w.Body.Len() != 0 {
		t.Fatalf("HEAD: status %d, body %q", w.Code, w.Body.String())
	}
}

func TestTankDownloadRejectsSymlinks(t *testing.T) {
	ts, id := testTankState(t)
	outside := filepath.Join(t.TempDir(), "secret")
	if err := os.WriteFile(outside, []byte("not a tank file"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := os.Symlink(outside, filepath.Join(ts.dir, id, "program")); err != nil {
		t.Skipf("symlinks unavailable: %v", err)
	}
	if err := os.Symlink(filepath.Join(ts.dir, id), filepath.Join(ts.dir, "linked-tank")); err != nil {
		t.Fatal(err)
	}
	for _, target := range []string{"/tanks/" + id + "/program", "/tanks/linked-tank/program"} {
		w := httptest.NewRecorder()
		ts.HTTPHandler().ServeHTTP(w, httptest.NewRequest(http.MethodGet, target, nil))
		if w.Code != http.StatusNotFound {
			t.Fatalf("%s: status %d, body %q", target, w.Code, w.Body.String())
		}
	}
}

func TestUploadedHTMLIsServedAsText(t *testing.T) {
	ts, id := testTankState(t)
	handler := ts.HTTPHandler()
	target := "/tanks/" + id + "/author"
	payload := `<!doctype html><script>document.title=localStorage.getItem("id")</script>`

	w := httptest.NewRecorder()
	handler.ServeHTTP(w, httptest.NewRequest(http.MethodPut, target, strings.NewReader(payload)))
	if w.Code != http.StatusOK {
		t.Fatalf("upload: status %d, body %q", w.Code, w.Body.String())
	}

	for _, method := range []string{http.MethodGet, http.MethodHead} {
		w := httptest.NewRecorder()
		handler.ServeHTTP(w, httptest.NewRequest(method, target, nil))
		if w.Code != http.StatusOK || w.Header().Get("Content-Type") != "text/plain; charset=utf-8" || w.Header().Get("X-Content-Type-Options") != "nosniff" {
			t.Fatalf("%s: status %d, headers %v", method, w.Code, w.Header())
		}
		if method == http.MethodGet && w.Body.String() != payload {
			t.Fatalf("download changed uploaded text: %q", w.Body.String())
		}
	}
}
