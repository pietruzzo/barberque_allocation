/*
 * Copyright (C) 2012  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dummy_plugin.h"
#include "dummy_test.h"
#include "bbque/plugins/static_plugin.h"

namespace bp = bbque::plugins;

#ifndef BBQUE_DYNAMIC_PLUGIN
# define	PLUGIN_NAME "dummy"
#else
# define	PLUGIN_NAME "dummy_dyn"
#endif

extern "C"
int32_t PF_exitFunc() {
  return 0;
}

extern "C"
PF_ExitFunc PF_initPlugin(const PF_PlatformServices * params) {
  int res = 0;


  PF_RegisterParams rp;
  rp.version.major = 1;
  rp.version.minor = 0;
  rp.programming_language = PF_LANG_CPP;

  // Registering DummyModule
  rp.CreateFunc = bp::DummyTest::Create;
  rp.DestroyFunc = bp::DummyTest::Destroy;
  res = params->RegisterObject((const char *)TEST_NAMESPACE PLUGIN_NAME, &rp);
  if (res < 0)
    return NULL;

  return PF_exitFunc;

}

#ifdef BBQUE_DYNAMIC_PLUGIN
PLUGIN_INIT(PF_initPlugin);
#else
bp::StaticPlugin
StaticPlugin_DummyTest(PF_initPlugin);
#endif

