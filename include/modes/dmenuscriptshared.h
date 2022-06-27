#ifndef ROFI_MODES_DMENU_SCRIPT_SHARED_H
#define ROFI_MODES_DMENU_SCRIPT_SHARED_H

#include <glib.h>
#include <mode.h>
#include <stdint.h>

typedef struct {
  /** Entry content. (visible part) */
  char *entry;
  /** Icon name to display. */
  char *icon_name;
  /** Async icon fetch handler. */
  uint32_t icon_fetch_uid;
  uint32_t icon_fetch_size;
  /** Hidden meta keywords. */
  char *meta;

  /** info */
  char *info;

  /** non-selectable */
  gboolean nonselectable;
} DmenuScriptEntry;
/**
 * @param sw Unused
 * @param entry The entry to update.
 * @param buffer The buffer to parse.
 * @param length The buffer length.
 *
 * Updates entry with the parsed values from buffer.
 */
void dmenuscript_parse_entry_extras(G_GNUC_UNUSED Mode *sw,
                                    DmenuScriptEntry *entry, char *buffer,
                                    size_t length);
#endif // ROFI_MODES_DMENU_SCRIPT_SHARED_H
