// Copyright 2022 Bas Wijnen <wijnen@debian.org>

#ifndef _earUI_base
#define _earUI_base

#include <Wt/WApplication.h>

class EarUI_base : public Wt::WApplication
{
public:
	EarUI_base(const Wt::WEnvironment& env) : Wt::WApplication (env) {}
	virtual void updateInputs() = 0;
};

#endif
