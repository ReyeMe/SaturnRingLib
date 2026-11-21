// Tests/campaigns/tilemap/main.cxx
// Tilemaps: TilemapInfo, SGLTile, Bmp2Tile conversion and map editing, and loading tilemaps into NBG1.
#include "campaign.hpp"

#include "testsTilemap.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(tilemap_test_suite);

    EndCampaign();
}
