package fetcher

import "strings"

type RobotsChecker struct {
	allowedPrefixes  []string
	disallowedPrefix []string
}

func NewRobotsChecker() *RobotsChecker {
	return &RobotsChecker{
		allowedPrefixes: []string{
			"/ru/wiki/",
			"/ru/api.php",
		},
		disallowedPrefix: []string{
			"/ru/wiki/Special:",
			"/ru/wiki/User",
			"/ru/wiki/Template",
			"/ru/wiki/Help",
		},
	}
}

func (r *RobotsChecker) Allowed(path string) bool {
	for _, dis := range r.disallowedPrefix {
		if strings.HasPrefix(path, dis) {
			return false
		}
	}
	for _, allow := range r.allowedPrefixes {
		if strings.HasPrefix(path, allow) {
			return true
		}
	}
	return false
}
