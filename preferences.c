/*
 * Copyright (c) 2013 Todd Mortimer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <unicode/utf.h>

#include "terminal.h"
#include "preferences.h"

static char* preferences_filename = ".term48rc";
static config_t* preferences;

config_t* preferences_get(){
	return preferences;
}


config_setting_t* preferences_init_string(config_setting_t* root, char* key, char* default_value){
  int type = CONFIG_TYPE_STRING;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting || config_setting_type(setting) != type){
    config_setting_remove(root, key);
    setting = config_setting_add(root, key, type);
    config_setting_set_string(setting, default_value);
  }
  return setting;
}

config_setting_t* preferences_init_int(config_setting_t* root, char* key, int default_value){
  int type = CONFIG_TYPE_INT;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting || config_setting_type(setting) != type){
    config_setting_remove(root, key);
    setting = config_setting_add(root, key, type);
    config_setting_set_int(setting, default_value);
  }
  return setting;
}

config_setting_t* preferences_init_bool(config_setting_t* root, char* key, int default_value){
  int type = CONFIG_TYPE_BOOL;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting || config_setting_type(setting) != type){
    config_setting_remove(root, key);
    setting = config_setting_add(root, key, type);
    config_setting_set_bool(setting, default_value);
  }
  return setting;
}

config_setting_t* preferences_init_group(config_setting_t* root, char* key){
  int type = CONFIG_TYPE_GROUP;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting || config_setting_type(setting) != type){
    config_setting_remove(root, key);
    setting = config_setting_add(root, key, type);
  }
  return setting;
}

config_setting_t* preferences_init_array(config_setting_t* root, char* key){
  int type = CONFIG_TYPE_ARRAY;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting || config_setting_type(setting) != type){
    config_setting_remove(root, key);
    setting = config_setting_add(root, key, type);
  }
  return setting;
}

void preferences_pad_array_int(config_setting_t* setting, int size, int padding){
	/* we assume that setting is actually a CONFIG_TYPE_ARRAY */
	int num = config_setting_length(setting);
	for(; num < size; ++num){
		config_setting_set_int_elem(setting, -1, padding);
	}
}

int preferences_check_version(config_setting_t* root, char* key){
  int type = CONFIG_TYPE_INT;
  config_setting_t* setting = config_setting_get_member(root, key);
  if(!setting /* not found */
  		|| (config_setting_type(setting) != type) /* has been messed with */
  		|| (config_setting_get_int(setting) != PREFS_VERSION)){ /* old */
  	/* we need to update */
  	config_setting_remove(root, key);
		setting = config_setting_add(root, key, type);
		config_setting_set_int(setting, PREFS_VERSION);
		return 1;
  }
  return 0;
}

int preferences_guess_best_font_size(){
	/* font widths in pixels for sizes 0-64, indexed by font size */
	int num_sizes = 65;
	int font_widths[65] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6,
			                   7, 7, 8, 8, 9, 10, 10, 11, 11, 12,
			                   13, 13, 14, 14, 15, 16, 16, 17, 17,
			                   18, 19, 19, 20, 20, 21, 22, 22, 23,
			                   23, 24, 25, 25, 26, 26, 27, 28, 28,
			                   29, 29, 30, 31, 31, 32, 32, 33, 34,
			                   34, 35, 35, 36, 37, 37, 38, 38};
	int target_cols = 60;
	int screen_width, screen_height, target_width;
	if((getenv("WIDTH") == NULL) || (getenv("HEIGHT") == NULL)){
		/* no width or height in env, just return the default */
		return preference_defaults.font_size;
	}
	screen_width = atoi(getenv("WIDTH"));
	screen_height = atoi(getenv("HEIGHT"));
	target_width = screen_width < screen_height ? screen_width : screen_height;
	int i = 0;
	int num_px = 0;
	for(i = 0; i < num_sizes; ++i){
		num_px = target_cols * font_widths[i];
		if(num_px > target_width){
			/* if we are too big, return the last one. */
			PRINT(stderr, "Autodetected font size %d for screen width %d\n", (i - 1), target_width);
			return (i - 1);
		}
	}
	/* if we get here, then just return the largest font */
	return (num_sizes - 1);
}

void preferences_init(){

  preferences = calloc(1, sizeof(config_t));
  char prefs_found = 0;

  config_init(preferences);
  FILE *f;
  f = fopen(preferences_filename, "r");
  if(f){
    fclose(f);
    prefs_found = 1;
    if(config_read_file(preferences, preferences_filename) != CONFIG_TRUE){
       fprintf(stderr, "%s:%d - %s\n", config_error_file(preferences),
                       config_error_line(preferences), config_error_text(preferences));
     }
  } else {
    PRINT(stderr, "Preferences file not found\n");
  }

  config_setting_t *root, *setting;
  root = config_root_setting(preferences);

  /* check for values from the conf file, and set them if not found or wrong type */
  preferences_init_string(root, preference_keys.font_path, preference_defaults.font_path);
  preferences_init_int(root, preference_keys.font_size, preferences_guess_best_font_size());
  preferences_init_bool(root, preference_keys.screen_idle_awake, preference_defaults.screen_idle_awake);
  preferences_init_bool(root, preference_keys.auto_show_vkb, preference_defaults.auto_show_vkb);
  preferences_init_bool(root, preference_keys.keyhold_actions, preference_defaults.keyhold_actions);
  preferences_init_int(root, preference_keys.metamode_hold_key, preference_defaults.metamode_hold_key);
  preferences_init_int(root, preference_keys.metamode_doubletap_key, preference_defaults.metamode_doubletap_key);
  preferences_init_int(root, preference_keys.metamode_doubletap_delay, preference_defaults.metamode_doubletap_delay);
  preferences_init_string(root, preference_keys.tty_encoding, preference_defaults.tty_encoding);

  setting = preferences_init_group(root, preference_keys.metamode_hitbox_s);
  preferences_init_int(setting, "x", preference_defaults.hitbox.x);
  preferences_init_int(setting, "y", preference_defaults.hitbox.y);
  preferences_init_int(setting, "w", preference_defaults.hitbox.w);
  preferences_init_int(setting, "h", preference_defaults.hitbox.h);

  /* init the keyhold_exempt keys */
  setting = config_setting_get_member(root, preference_keys.keyhold_actions_exempt);
  if(!setting || config_setting_type(setting) != CONFIG_TYPE_ARRAY){
  	setting = preferences_init_array(root, preference_keys.keyhold_actions_exempt);
  	int num_key_defaults = sizeof(preference_defaults.keyhold_actions_exempt) / sizeof(int);
  	int i = 0;
  	for(i = 0; i < num_key_defaults; ++i){
  		config_setting_set_int_elem(setting, -1, preference_defaults.keyhold_actions_exempt[i]);
  	}
  }

  /* initialize the text and background colours */
  setting = config_setting_get_member(root, preference_keys.text_color);
  if(!setting || config_setting_type(setting) != CONFIG_TYPE_ARRAY){
  	setting = preferences_init_array(root, preference_keys.text_color);
  	int i = 0;
  	for(i = 0; i < PREFS_COLOR_NUM_ELEMENTS; ++i){
  		config_setting_set_int_elem(setting, -1, preference_defaults.text_color[i]);
  	}
  }
  /* ensure array length is long enough */
  preferences_pad_array_int(setting, PREFS_COLOR_NUM_ELEMENTS, 0);

  setting = config_setting_get_member(root, preference_keys.background_color);
  if(!setting || config_setting_type(setting) != CONFIG_TYPE_ARRAY){
  	setting = preferences_init_array(root, preference_keys.background_color);
  	int i = 0;
  	for(i = 0; i < PREFS_COLOR_NUM_ELEMENTS; ++i){
  		config_setting_set_int_elem(setting, -1, preference_defaults.background_color[i]);
  	}
  }
  preferences_pad_array_int(setting, PREFS_COLOR_NUM_ELEMENTS, 0);

  /* initialize the metamode keys */
  setting = config_setting_get_member(root, preference_keys.metamode_keys);
  if(!setting || config_setting_type(setting) != CONFIG_TYPE_GROUP){
  	/* initialize the keystrokes if there are none defined */
  	setting = preferences_init_group(root, preference_keys.metamode_keys);
  	int num_key_defaults = sizeof(preference_defaults.metamode_keys) / sizeof(char*) / 2;
  	int i = 0;
  	for(i = 0; i < num_key_defaults; ++i){
  		preferences_init_string(setting,
  				preference_defaults.metamode_keys[i*2],
  				preference_defaults.metamode_keys[(i*2)+1]);
  	}
  }

  /* initialize the metamode sticky keys */
  setting = config_setting_get_member(root, preference_keys.metamode_sticky_keys);
	if(!setting || config_setting_type(setting) != CONFIG_TYPE_GROUP){
		/* initialize the keystrokes if there are none defined */
		setting = preferences_init_group(root, preference_keys.metamode_sticky_keys);
		int num_key_defaults = sizeof(preference_defaults.metamode_sticky_keys) / sizeof(char*) / 2;
		int i = 0;
		for(i = 0; i < num_key_defaults; ++i){
			preferences_init_string(setting,
				                      preference_defaults.metamode_sticky_keys[i*2],
				                      preference_defaults.metamode_sticky_keys[(i*2)+1]);
		}
	}

  /* initialize the metamode sticky keys */
  setting = config_setting_get_member(root, preference_keys.metamode_func_keys);
	if(!setting || config_setting_type(setting) != CONFIG_TYPE_GROUP){
		/* initialize the keystrokes if there are none defined */
		setting = preferences_init_group(root, preference_keys.metamode_func_keys);
		int num_key_defaults = sizeof(preference_defaults.metamode_func_keys) / sizeof(char*) / 2;
		int i = 0;
		for(i = 0; i < num_key_defaults; ++i){
			preferences_init_string(setting,
					preference_defaults.metamode_func_keys[i*2],
					preference_defaults.metamode_func_keys[(i*2)+1]);
		}
	}

  /* write the prefs file if there was no local one found or we have updated */
  if(preferences_check_version(root, preference_keys.prefs_version) || !prefs_found){
  	fprintf(stderr, "Writing prefs file\n");
    config_write_file(preferences, preferences_filename);
    /* This is probably a first run */
    first_run();
  }
}

int preferences_save(config_t* prefs){

  return 0;
}

int preferences_free(config_t* prefs){

  if(prefs != NULL){
    config_destroy(prefs);
    free(prefs);
  }
  return 0;
}

void preferences_uninit(){
	preferences_free(preferences);
}

const char* preferences_get_string(char* pref){
	if(!preferences){
		preferences_init();
	}
	const char* value;
  config_lookup_string(preferences, pref, &value);
  return value;
}

int preferences_get_int(char* pref){
	if(!preferences){
		preferences_init();
	}
	int value;
  config_lookup_int(preferences, pref, &value);
  return value;
}

int preferences_get_bool(char* pref){
	if(!preferences){
		preferences_init();
	}
	int value;
	config_lookup_bool(preferences, pref, &value);
	return value;
}

int preferences_get_int_array(char* pref, int* fillme, int length){
	config_setting_t *root, *setting;
	if(!preferences){
		preferences_init();
	}
	int num_copied = 0;
	root = config_root_setting(preferences);
  setting = config_setting_get_member(root, pref);
  if(setting && config_setting_is_array(setting)){
  	int i = 0;
  	int num_eles = config_setting_length(setting);
  	num_copied = length;
  	if(num_eles < length){
  		num_copied = num_eles;
  	}
  	for(i = 0; i < num_copied; ++i){
  		fillme[i] = config_setting_get_int_elem(setting, i);
  	}
  }
  return num_copied;
}

const char* preferences_get_metamode_keystrokes(char keystroke, char* pref_key){
	config_setting_t *keystrokes_group, *root;
	char setting_str[2] = {keystroke, 0};
	const char* setting_value;
	root = config_root_setting(preferences);
	keystrokes_group = config_setting_get_member(root, pref_key);
	if(keystrokes_group){
		if(CONFIG_TRUE == config_setting_lookup_string(keystrokes_group, setting_str, &setting_value)){
			return setting_value;
		}
	}
	// else
	return NULL;
}

const char* preferences_get_metamode_keys(char keystroke){
	return preferences_get_metamode_keystrokes(keystroke, preference_keys.metamode_keys);
}

const char* preferences_get_metamode_sticky_keys(char keystroke){
	return preferences_get_metamode_keystrokes(keystroke, preference_keys.metamode_sticky_keys);
}

const char* preferences_get_metamode_func_keys(char keystroke){
	return preferences_get_metamode_keystrokes(keystroke, preference_keys.metamode_func_keys);
}

int preferences_is_keyhold_exempt(int keystroke){
	config_setting_t *exempt, *root;
	root = config_root_setting(preferences);
	exempt = config_setting_get_member(root, preference_keys.keyhold_actions_exempt);
	if(exempt){
		int num_elements = config_setting_length(exempt);
		int i = 0;
		for(i = 0; i < num_elements; ++i){
			if(keystroke == config_setting_get_int_elem(exempt, i)){
				return 1;
			}
		}
	}
	// else
	return NULL;
}

