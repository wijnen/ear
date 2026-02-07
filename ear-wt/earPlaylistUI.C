/*
 * Example used: Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * Code copyright Kasper Loopstra, 2019
 * Copyright 2022 Bas Wijnen <wijnen@debian.org>
 */

#include <Wt/WLogger.h>
#include <Wt/WBootstrap3Theme.h>
#include "earUI_base.h"
#include <Wt/WTreeTable.h>
#include <Wt/WTimer.h>
#include <Wt/Json/Object.h>
#include <Wt/WTemplate.h>
#include <Wt/WEnvironment.h>
#include "earzmq.h"
#include "trackSearchContainer.h"

class EarPlaylistUI : public EarUI_base
{
public:
	EarPlaylistUI(const Wt::WEnvironment& env);
private:
	TrackSearchContainer *trackSearchContainer;
	int ui_track_idx;
	void updateInputs();
};


EarPlaylistUI::EarPlaylistUI(const Wt::WEnvironment& env) : EarUI_base(env) {
	setTitle("Ear Playlist/ballroom interface");
	auto theme = std::make_shared <Wt::WBootstrap3Theme> ();
	theme->setResponsive(true);
	setTheme(theme);

	root()->addWidget(std::make_unique <TrackSearchContainer> (this));
}

void EarPlaylistUI::updateInputs() {
}

int main(int argc, char **argv) {
	Wt::WString::setDefaultEncoding(Wt::CharEncoding::UTF8);
	return Wt::WRun(argc, argv, [](Wt::WEnvironment const &env) {
		return std::make_unique <EarPlaylistUI> (env);
	});
}
