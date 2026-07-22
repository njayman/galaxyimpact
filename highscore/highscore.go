package highscore

import (
	"os"
	"sort"
	"strconv"
	"strings"
)

const maxEntries = 5

type Repository interface {
	Load() ([]int32, error)
	Save(scores []int32) error
}

type FileRepository struct {
	Path string
}

func NewFileRepository(path string) *FileRepository {
	return &FileRepository{Path: path}
}

func (r *FileRepository) Load() ([]int32, error) {
	data, err := os.ReadFile(r.Path)
	if os.IsNotExist(err) {
		return []int32{}, nil
	}
	if err != nil {
		return nil, err
	}

	var scores []int32
	for _, line := range strings.Split(strings.TrimSpace(string(data)), "\n") {
		if line == "" {
			continue
		}
		n, err := strconv.Atoi(line)
		if err != nil {
			continue
		}
		scores = append(scores, int32(n))
	}

	return scores, nil
}

func (r *FileRepository) Save(scores []int32) error {
	lines := make([]string, len(scores))
	for i, s := range scores {
		lines[i] = strconv.Itoa(int(s))
	}

	return os.WriteFile(r.Path, []byte(strings.Join(lines, "\n")), 0644)
}

// Record inserts score into scores, keeps it sorted descending, truncates to
// the top maxEntries, persists via repo, and returns the updated slice.
func Record(repo Repository, scores []int32, score int32) []int32 {
	updated := append(append([]int32{}, scores...), score)

	sort.Slice(updated, func(i, j int) bool { return updated[i] > updated[j] })

	if len(updated) > maxEntries {
		updated = updated[:maxEntries]
	}

	repo.Save(updated)

	return updated
}
