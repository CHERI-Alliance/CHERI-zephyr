/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VISITORS_H
#define VISITORS_H

#include "clang/Tooling/Tooling.h"

std::unique_ptr<clang::tooling::FrontendActionFactory> createPass1ActionFactory();
std::unique_ptr<clang::tooling::FrontendActionFactory> createPass2ActionFactory();
std::unique_ptr<clang::tooling::FrontendActionFactory> createPass3ActionFactory();

#endif