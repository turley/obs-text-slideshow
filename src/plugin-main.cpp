/*
obs-text-slideshow
Copyright (C) 2021 Joshua Wong jbwong05@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>

#include "plugin-macros.generated.h"
#include "obs-text-slideshow-dock.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern void load_text_freetype2_slideshow();
#ifdef _WIN32
extern void load_text_gdiplus_slideshow();
#endif

bool obs_module_load(void)
{
	return true;
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		const auto main_window =
			static_cast<QMainWindow *>(obs_frontend_get_main_window());
		obs_frontend_push_ui_translation(obs_module_get_string);
		auto *tmp = new TextSlideshowDock(main_window);
		obs_frontend_add_dock(tmp);
		obs_frontend_pop_ui_translation();
	}
}

void obs_module_post_load(void)
{
	obs_frontend_add_event_callback(OBSEvent, nullptr);

	load_text_freetype2_slideshow();
#ifdef _WIN32
	load_text_gdiplus_slideshow();
#endif
	blog(LOG_INFO, "plugin loaded successfully (version %s)",
		PLUGIN_VERSION);
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}
