#pragma once

#include "LadderConfig.h"
#include "Types.h"

class Webservice{
public:
    // Just for now these will be passed the config because it was quicker this way.
    bool LoginToServer(LadderConfig &Config);

    bool UploadMime(LadderConfig &Config, ResultType result, const Matchup &ThisMatch);
};
