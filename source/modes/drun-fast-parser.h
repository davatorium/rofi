/*
 * rofi - Fast Desktop Entry Parser
 *
 * MIT/X11 License
 * Copyright 2013-2024 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DRUN_FAST_PARSER_H
#define DRUN_FAST_PARSER_H

#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * FAST DESKTOP ENTRY PARSER
 *
 * This parser is optimized for speed while supporting all needed features:
 * 1. AVX2 SIMD for "[Desktop Entry]" section search (when available)
 * 2. Single-pass parsing with minimal allocations
 * 3. Locale-aware key matching (Name[en_US] > Name)
 * 4. Boolean and string list parsing
 * ============================================================================ */

#ifdef __AVX2__
#include <immintrin.h>

/* AVX2-optimized "[Desktop Entry]" search */
__attribute__((target("avx2")))
static inline const char *dfp_find_section_avx2(const char *d, size_t len) {
  if (len < 15) return NULL;

  const __m256i bracket = _mm256_set1_epi8('[');
  const __m256i desk = _mm256_loadu_si256((const __m256i*)"Desktop Entry]xx");
  const char *end = d + len - 32;

  for (const char *p = d; p <= end; p += 32) {
    __m256i chunk = _mm256_loadu_si256((const __m256i*)p);
    unsigned int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, bracket));

    while (mask) {
      int idx = __builtin_ctz(mask);
      mask &= mask - 1;
      const char *c = p + idx;
      if (c + 15 > d + len) continue;

      __m256i cand = _mm256_loadu_si256((const __m256i*)(c + 1));
      unsigned int m2 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(cand, desk));
      if ((m2 & 0x3FFF) == 0x3FFF) return c;
    }
  }

  for (const char *p = (d + len - 31 > d ? d + len - 31 : d); p <= d + len - 15; p++) {
    if (*p == '[' && __builtin_memcmp(p + 1, "Desktop Entry]", 14) == 0) return p;
  }
  return NULL;
}

static gboolean dfp_has_avx2 = FALSE;
#endif

/* Scalar fallback using 8-byte loads */
static inline const char *dfp_find_section_scalar(const char * __restrict d, size_t len) {
  if (len < 15) return NULL;

  const uint64_t magic1 = 0x20706F746B736544ULL; /* "Desktop " */
  const uint64_t magic2 = 0x00005D7972746E45ULL; /* "Entry]" */

  for (const char *p = d, *e = d + len - 14; p <= e; p++) {
    if (*p != '[') continue;
    uint64_t v1, v2;
    __builtin_memcpy(&v1, p + 1, 8);
    if (v1 != magic1) continue;
    __builtin_memcpy(&v2, p + 9, 6);
    if (v2 == magic2) return p;
  }
  return NULL;
}

/* Runtime-selected section finder */
static const char *(*dfp_find_section)(const char*, size_t) = dfp_find_section_scalar;

/* Initialize fast parser (call once at startup) */
static inline void dfp_init(void) {
#ifdef __AVX2__
  unsigned int eax = 7, ebx, ecx = 0, edx;
  __asm__ __volatile__("cpuid" : "+a"(eax), "=b"(ebx), "+c"(ecx), "=d"(edx));
  dfp_has_avx2 = (ebx & (1 << 5)) != 0;
  if (dfp_has_avx2) {
    dfp_find_section = dfp_find_section_avx2;
  }
#endif
}

/* Parsed desktop entry data */
typedef struct {
  /* Strings (all allocated) */
  char *type;
  char *name;
  char *name_locale;      /* Name[lang] if found */
  char *generic_name;
  char *generic_name_locale;
  char *exec;
  char *icon;
  char *icon_locale;
  char *comment;
  char *comment_locale;
  char *url;
  char *url_locale;
  char *try_exec;

  /* String lists */
  char **categories;
  char **keywords;
  char **only_show_in;
  char **not_show_in;
  char **actions;

  /* Booleans */
  unsigned int hidden : 1;
  unsigned int no_display : 1;
  unsigned int terminal : 1;
  unsigned int has_type : 1;
  unsigned int has_name : 1;
  unsigned int has_exec : 1;

  /* Raw pointer for action group parsing (not owned) */
  const char *raw_start;
  size_t raw_len;
} DFEntry;

/* Free all allocated memory in a DFEntry */
static inline void dfp_entry_clear(DFEntry *e) {
  if (!e) return;
  g_free(e->type);
  g_free(e->name);
  g_free(e->name_locale);
  g_free(e->generic_name);
  g_free(e->generic_name_locale);
  g_free(e->exec);
  g_free(e->icon);
  g_free(e->icon_locale);
  g_free(e->comment);
  g_free(e->comment_locale);
  g_free(e->url);
  g_free(e->url_locale);
  g_free(e->try_exec);
  g_strfreev(e->categories);
  g_strfreev(e->keywords);
  g_strfreev(e->only_show_in);
  g_strfreev(e->not_show_in);
  g_strfreev(e->actions);
  memset(e, 0, sizeof(*e));
}

/* Get best name (locale > generic) */
static inline const char *dfp_get_name(const DFEntry *e) {
  return e->name_locale ? e->name_locale : e->name;
}

static inline const char *dfp_get_generic_name(const DFEntry *e) {
  return e->generic_name_locale ? e->generic_name_locale : e->generic_name;
}

static inline const char *dfp_get_icon(const DFEntry *e) {
  return e->icon_locale ? e->icon_locale : e->icon;
}

static inline const char *dfp_get_comment(const DFEntry *e) {
  return e->comment_locale ? e->comment_locale : e->comment;
}

static inline const char *dfp_get_url(const DFEntry *e) {
  return e->url_locale ? e->url_locale : e->url;
}

/* Check if entry is valid Application/Link/Service */
static inline int dfp_is_valid_entry(const DFEntry *e) {
  if (e->hidden || e->no_display) return 0;
  if (!e->has_type || !e->has_name) return 0;

  if (g_strcmp0(e->type, "Application") == 0) {
    return e->has_exec;
  }
  if (g_strcmp0(e->type, "Link") == 0) {
    return e->url != NULL || e->url_locale != NULL;
  }
  if (g_strcmp0(e->type, "Service") == 0) {
    return e->has_exec;
  }
  return 0;
}

/* Case-insensitive ASCII comparison for key matching */
static inline int dfp_key_eq(const char *a, const char *b, size_t len) {
  for (size_t i = 0; i < len; i++) {
    unsigned char ca = a[i], cb = b[i];
    if (ca >= 'A' && ca <= 'Z') ca += 32;
    if (cb >= 'A' && cb <= 'Z') cb += 32;
    if (ca != cb) return 0;
  }
  return 1;
}

/* Duplicate a value, stripping trailing whitespace */
static inline char *dfp_dup_value(const char *v, size_t len) {
  while (len > 0 && (v[len-1] == ' ' || v[len-1] == '\t' ||
                     v[len-1] == '\r' || v[len-1] == '\n')) {
    len--;
  }
  if (len == 0) return NULL;
  return g_strndup(v, len);
}

/* Parse a semicolon-separated list */
static inline char **dfp_parse_list(const char *v, size_t len) {
  /* Count items */
  size_t count = 0;
  const char *p = v, *e = v + len;
  while (p < e) {
    while (p < e && (*p == ' ' || *p == '\t')) p++;
    if (p >= e || *p == ';') { if (p < e) p++; continue; }
    count++;
    while (p < e && *p != ';') p++;
    if (p < e) p++;
  }

  if (count == 0) return NULL;

  char **list = g_new0(char*, count + 1);
  size_t idx = 0;
  p = v;
  while (p < e && idx < count) {
    while (p < e && (*p == ' ' || *p == '\t')) p++;
    if (p >= e || *p == ';') { if (p < e) p++; continue; }

    const char *start = p;
    while (p < e && *p != ';') p++;
    size_t item_len = p - start;
    while (item_len > 0 && (start[item_len-1] == ' ' || start[item_len-1] == '\t')) {
      item_len--;
    }
    if (item_len > 0) {
      list[idx++] = g_strndup(start, item_len);
    }
    if (p < e) p++;
  }

  return list;
}

/* Parse a boolean value */
static inline int dfp_parse_bool(const char *v, size_t len) {
  if (len == 0) return 0;
  return (v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 'y' || v[0] == 'Y');
}

/* Key type enumeration for fast dispatch */
typedef enum {
  DFP_KEY_UNKNOWN = 0,
  DFP_KEY_TYPE,
  DFP_KEY_NAME,
  DFP_KEY_GENERIC_NAME,
  DFP_KEY_EXEC,
  DFP_KEY_ICON,
  DFP_KEY_COMMENT,
  DFP_KEY_URL,
  DFP_KEY_TRY_EXEC,
  DFP_KEY_CATEGORIES,
  DFP_KEY_KEYWORDS,
  DFP_KEY_ONLY_SHOW_IN,
  DFP_KEY_NOT_SHOW_IN,
  DFP_KEY_ACTIONS,
  DFP_KEY_HIDDEN,
  DFP_KEY_NO_DISPLAY,
  DFP_KEY_TERMINAL,
} DFPKeyType;

/* Quick key identification by first character */
static inline DFPKeyType dfp_identify_key(const char *key, size_t key_len,
                                          const char **locale_out, size_t *locale_len) {
  *locale_out = NULL;
  *locale_len = 0;

  /* Check for locale suffix [xx_XX] */
  const char *locale_start = NULL;
  for (size_t i = 0; i < key_len; i++) {
    if (key[i] == '[') {
      locale_start = key + i + 1;
      for (size_t j = i + 1; j < key_len; j++) {
        if (key[j] == ']') {
          *locale_out = locale_start;
          *locale_len = j - i - 1;
          key_len = i;  /* Key name ends at [ */
          break;
        }
      }
      break;
    }
  }

  if (key_len == 0) return DFP_KEY_UNKNOWN;

  /* Fast dispatch on first character */
  switch (key[0]) {
    case 'T': case 't':
      if (key_len == 4 && dfp_key_eq(key, "Type", 4)) return DFP_KEY_TYPE;
      if (key_len == 7 && dfp_key_eq(key, "TryExec", 7)) return DFP_KEY_TRY_EXEC;
      if (key_len == 8 && dfp_key_eq(key, "Terminal", 8)) return DFP_KEY_TERMINAL;
      break;
    case 'N': case 'n':
      if (key_len == 4 && dfp_key_eq(key, "Name", 4)) return DFP_KEY_NAME;
      if (key_len == 8 && dfp_key_eq(key, "NoDisplay", 9)) return DFP_KEY_NO_DISPLAY;
      break;
    case 'G': case 'g':
      if (key_len == 11 && dfp_key_eq(key, "GenericName", 11)) return DFP_KEY_GENERIC_NAME;
      break;
    case 'E': case 'e':
      if (key_len == 4 && dfp_key_eq(key, "Exec", 4)) return DFP_KEY_EXEC;
      break;
    case 'I': case 'i':
      if (key_len == 4 && dfp_key_eq(key, "Icon", 4)) return DFP_KEY_ICON;
      break;
    case 'C': case 'c':
      if (key_len == 7 && dfp_key_eq(key, "Comment", 7)) return DFP_KEY_COMMENT;
      if (key_len == 10 && dfp_key_eq(key, "Categories", 10)) return DFP_KEY_CATEGORIES;
      break;
    case 'U': case 'u':
      if (key_len == 3 && dfp_key_eq(key, "URL", 3)) return DFP_KEY_URL;
      break;
    case 'K': case 'k':
      if (key_len == 8 && dfp_key_eq(key, "Keywords", 8)) return DFP_KEY_KEYWORDS;
      break;
    case 'O': case 'o':
      if (key_len == 10 && dfp_key_eq(key, "OnlyShowIn", 10)) return DFP_KEY_ONLY_SHOW_IN;
      break;
    case 'H': case 'h':
      if (key_len == 6 && dfp_key_eq(key, "Hidden", 6)) return DFP_KEY_HIDDEN;
      break;
    case 'A': case 'a':
      if (key_len == 7 && dfp_key_eq(key, "Actions", 7)) return DFP_KEY_ACTIONS;
      break;
  }

  /* Second pass for keys with different first chars */
  if (key[0] == 'N' || key[0] == 'n') {
    if (key_len == 9 && dfp_key_eq(key, "NoDisplay", 9)) return DFP_KEY_NO_DISPLAY;
    if (key_len == 10 && dfp_key_eq(key, "NotShowIn", 10)) return DFP_KEY_NOT_SHOW_IN;
  }

  return DFP_KEY_UNKNOWN;
}

/* Main parser function */
static inline int dfp_parse(const char *data, size_t len, DFEntry *entry,
                            const char * const *locales) {
  memset(entry, 0, sizeof(*entry));
  entry->raw_start = data;
  entry->raw_len = len;

  /* Find [Desktop Entry] section */
  const char *section = dfp_find_section(data, len);
  if (!section) return 0;

  /* Track best locale match for each key type */
  int name_locale_score = -1;
  int generic_name_locale_score = -1;
  int icon_locale_score = -1;
  int comment_locale_score = -1;
  int url_locale_score = -1;

  /* Count locales for scoring */
  int num_locales = 0;
  if (locales) {
    while (locales[num_locales]) num_locales++;
  }

  /* Parse line by line */
  const char *p = section + 15;  /* Skip "[Desktop Entry]" */
  const char *end = data + len;

  while (p < end) {
    /* Find end of line */
    const char *nl = p;
    while (nl < end && *nl != '\n') nl++;

    /* Skip leading whitespace */
    while (p < nl && (*p == ' ' || *p == '\t')) p++;

    /* Skip empty lines and comments */
    if (p >= nl || *p == '#') {
      p = nl + 1;
      continue;
    }

    /* Check for new section (ends parsing) */
    if (*p == '[') {
      break;
    }

    /* Find key=value separator */
    const char *eq = p;
    while (eq < nl && *eq != '=') eq++;

    if (eq >= nl) {
      p = nl + 1;
      continue;  /* No = found */
    }

    const char *key = p;
    size_t key_len = eq - key;
    const char *value = eq + 1;
    size_t value_len = nl - value;

    /* Identify key type */
    const char *locale = NULL;
    size_t locale_len = 0;
    DFPKeyType kt = dfp_identify_key(key, key_len, &locale, &locale_len);

    /* Calculate locale match score */
    int locale_score = -1;  /* -1 = no locale, 0+ = matched locale index */
    if (locale && locales) {
      for (int i = 0; i < num_locales; i++) {
        size_t ll = strlen(locales[i]);
        /* Exact match or prefix match (en_US matches en) */
        if ((locale_len == ll && memcmp(locale, locales[i], ll) == 0) ||
            (ll < locale_len && locale[ll] == '_' &&
             memcmp(locale, locales[i], ll) == 0)) {
          locale_score = i;
          break;
        }
      }
      /* If no locale matched, skip this localized key */
      if (locale_score < 0) {
        p = nl + 1;
        continue;
      }
    }

    /* Store value based on key type */
    switch (kt) {
      case DFP_KEY_TYPE:
        g_free(entry->type);
        entry->type = dfp_dup_value(value, value_len);
        entry->has_type = 1;
        break;

      case DFP_KEY_NAME:
        if (locale && locale_score >= 0) {
          if (locale_score > name_locale_score) {
            g_free(entry->name_locale);
            entry->name_locale = dfp_dup_value(value, value_len);
            name_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->name);
          entry->name = dfp_dup_value(value, value_len);
          entry->has_name = 1;
        }
        break;

      case DFP_KEY_GENERIC_NAME:
        if (locale && locale_score >= 0) {
          if (locale_score > generic_name_locale_score) {
            g_free(entry->generic_name_locale);
            entry->generic_name_locale = dfp_dup_value(value, value_len);
            generic_name_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->generic_name);
          entry->generic_name = dfp_dup_value(value, value_len);
        }
        break;

      case DFP_KEY_EXEC:
        g_free(entry->exec);
        entry->exec = dfp_dup_value(value, value_len);
        entry->has_exec = 1;
        break;

      case DFP_KEY_ICON:
        if (locale && locale_score >= 0) {
          if (locale_score > icon_locale_score) {
            g_free(entry->icon_locale);
            entry->icon_locale = dfp_dup_value(value, value_len);
            icon_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->icon);
          entry->icon = dfp_dup_value(value, value_len);
        }
        break;

      case DFP_KEY_COMMENT:
        if (locale && locale_score >= 0) {
          if (locale_score > comment_locale_score) {
            g_free(entry->comment_locale);
            entry->comment_locale = dfp_dup_value(value, value_len);
            comment_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->comment);
          entry->comment = dfp_dup_value(value, value_len);
        }
        break;

      case DFP_KEY_URL:
        if (locale && locale_score >= 0) {
          if (locale_score > url_locale_score) {
            g_free(entry->url_locale);
            entry->url_locale = dfp_dup_value(value, value_len);
            url_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->url);
          entry->url = dfp_dup_value(value, value_len);
        }
        break;

      case DFP_KEY_TRY_EXEC:
        g_free(entry->try_exec);
        entry->try_exec = dfp_dup_value(value, value_len);
        break;

      case DFP_KEY_CATEGORIES:
        g_strfreev(entry->categories);
        entry->categories = dfp_parse_list(value, value_len);
        break;

      case DFP_KEY_KEYWORDS:
        g_strfreev(entry->keywords);
        entry->keywords = dfp_parse_list(value, value_len);
        break;

      case DFP_KEY_ONLY_SHOW_IN:
        g_strfreev(entry->only_show_in);
        entry->only_show_in = dfp_parse_list(value, value_len);
        break;

      case DFP_KEY_NOT_SHOW_IN:
        g_strfreev(entry->not_show_in);
        entry->not_show_in = dfp_parse_list(value, value_len);
        break;

      case DFP_KEY_ACTIONS:
        g_strfreev(entry->actions);
        entry->actions = dfp_parse_list(value, value_len);
        break;

      case DFP_KEY_HIDDEN:
        entry->hidden = dfp_parse_bool(value, value_len);
        break;

      case DFP_KEY_NO_DISPLAY:
        entry->no_display = dfp_parse_bool(value, value_len);
        break;

      case DFP_KEY_TERMINAL:
        entry->terminal = dfp_parse_bool(value, value_len);
        break;

      default:
        break;
    }

    p = nl + 1;
  }

  return 1;
}

/* Parse a Desktop Action group (e.g., [Desktop Action NewWindow]) */
static inline int dfp_parse_action(const char *data, size_t len,
                                   const char *action_name,
                                   DFEntry *entry,
                                   const char * const *locales) {
  memset(entry, 0, sizeof(*entry));
  entry->raw_start = data;
  entry->raw_len = len;

  /* Find the action group */
  size_t action_len = strlen(action_name);
  const char *p = data;
  const char *end = data + len;

  while (p < end) {
    const char *nl = p;
    while (nl < end && *nl != '\n') nl++;

    if (p < nl && *p == '[') {
      size_t group_len = nl - p - 1;  /* Exclude ] */
      if (group_len > 0 && p[group_len] == ']') {
        if (group_len == action_len && memcmp(p + 1, action_name, action_len) == 0) {
          /* Found the action group, parse it */
          p = nl + 1;
          goto parse_group;
        }
      }
    }
    p = nl + 1;
  }
  return 0;  /* Group not found */

parse_group:
  ; /* Empty statement for C compatibility */
  /* Track best locale match */
  int name_locale_score = -1;

  /* Count locales for scoring */
  int num_locales = 0;
  if (locales) {
    while (locales[num_locales]) num_locales++;
  }

  while (p < end) {
    const char *nl = p;
    while (nl < end && *nl != '\n') nl++;

    while (p < nl && (*p == ' ' || *p == '\t')) p++;

    if (p >= nl || *p == '#') {
      p = nl + 1;
      continue;
    }

    if (*p == '[') {
      break;  /* New group, stop */
    }

    const char *eq = p;
    while (eq < nl && *eq != '=') eq++;

    if (eq >= nl) {
      p = nl + 1;
      continue;
    }

    const char *key = p;
    size_t key_len = eq - key;
    const char *value = eq + 1;
    size_t value_len = nl - value;

    const char *locale = NULL;
    size_t locale_len = 0;
    DFPKeyType kt = dfp_identify_key(key, key_len, &locale, &locale_len);

    int locale_score = -1;
    if (locale && locales) {
      for (int i = 0; i < num_locales; i++) {
        size_t ll = strlen(locales[i]);
        if ((locale_len == ll && memcmp(locale, locales[i], ll) == 0) ||
            (ll < locale_len && locale[ll] == '_' &&
             memcmp(locale, locales[i], ll) == 0)) {
          locale_score = i;
          break;
        }
      }
      if (locale_score < 0) {
        p = nl + 1;
        continue;
      }
    }

    switch (kt) {
      case DFP_KEY_NAME:
        if (locale && locale_score >= 0) {
          if (locale_score > name_locale_score) {
            g_free(entry->name_locale);
            entry->name_locale = dfp_dup_value(value, value_len);
            name_locale_score = locale_score;
          }
        } else if (!locale) {
          g_free(entry->name);
          entry->name = dfp_dup_value(value, value_len);
        }
        break;

      case DFP_KEY_EXEC:
        g_free(entry->exec);
        entry->exec = dfp_dup_value(value, value_len);
        break;

      case DFP_KEY_ICON:
        if (locale && locale_score >= 0) {
          if (!entry->icon_locale || locale_score > 0) {
            g_free(entry->icon_locale);
            entry->icon_locale = dfp_dup_value(value, value_len);
          }
        } else if (!locale) {
          g_free(entry->icon);
          entry->icon = dfp_dup_value(value, value_len);
        }
        break;

      default:
        break;
    }

    p = nl + 1;
  }

  return (entry->name || entry->name_locale) && entry->exec;
}

#endif /* DRUN_FAST_PARSER_H */
