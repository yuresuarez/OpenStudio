/***********************************************************************************************************************
*  OpenStudio(R), Copyright (c) 2008-2022, Alliance for Sustainable Energy, LLC, and other contributors. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
*  disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote products
*  derived from this software without specific prior written permission from the respective party.
*
*  (4) Other than as required in clauses (1) and (2), distributions in any form of modifications or other derivative works
*  may not use the "OpenStudio" trademark, "OS", "os", or any other confusingly similar designation without specific prior
*  written permission from Alliance for Sustainable Energy, LLC.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND ANY CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S), ANY CONTRIBUTORS, THE UNITED STATES GOVERNMENT, OR THE UNITED
*  STATES DEPARTMENT OF ENERGY, NOR ANY OF THEIR EMPLOYEES, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************************************************************/

#include <gtest/gtest.h>

#include "ModelFixture.hpp"

#include "../ThreeJSReverseTranslator.hpp"
#include "../ThreeJSForwardTranslator.hpp"
#include "../FloorspaceReverseTranslator.hpp"
#include "../ModelMerger.hpp"
#include "../Model.hpp"
#include "../BuildingStory.hpp"
#include "../BuildingStory_Impl.hpp"
#include "../DaylightingControl.hpp"
#include "../DaylightingControl_Impl.hpp"
#include "../Space.hpp"
#include "../Space_Impl.hpp"
#include "../ShadingSurfaceGroup.hpp"
#include "../ShadingSurfaceGroup_Impl.hpp"
#include "../ThermalZone.hpp"
#include "../ThermalZone_Impl.hpp"
#include "../SpaceType.hpp"
#include "../SpaceType_Impl.hpp"
#include "../Surface.hpp"
#include "../Surface_Impl.hpp"
#include "../SubSurface.hpp"
#include "../SubSurface_Impl.hpp"
#include "../ShadingSurface.hpp"
#include "../ShadingSurface_Impl.hpp"
#include "../DefaultConstructionSet.hpp"
#include "../DefaultConstructionSet_Impl.hpp"
#include "../RenderingColor.hpp"
#include "../RenderingColor_Impl.hpp"
#include "../ClimateZones.hpp"
#include "../ClimateZones_Impl.hpp"
#include "../Site.hpp"
#include "../Site_Impl.hpp"
#include "../Building.hpp"
#include "../Building_Impl.hpp"
#include "../BuildingUnit.hpp"
#include "../BuildingUnit_Impl.hpp"
#include "../../utilities/geometry/ThreeJS.hpp"
#include "../../utilities/geometry/FloorplanJS.hpp"
#include "../../utilities/geometry/Geometry.hpp"
#include "../../utilities/units/QuantityConverter.hpp"
#include <boost/optional/optional_io.hpp>

using namespace openstudio;
using namespace openstudio::model;

#define DBOUT(s)                          \
  {                                       \
    std::ostringstream os_;               \
    os_ << s;                             \
    OutputDebugString(os_.str().c_str()); \
  }

void CompareTwoModels(Model& model, Model& baseline) {

  // Compare story count
  // Compare stories
  //   Match by name,compare properties
  //   Compare shading count
  double tol = 0.001;  // Same tolerance as defaults for circularEqual
  auto thermalZones1 = model.getModelObjects<ThermalZone>();
  auto thermalZones2 = baseline.getModelObjects<ThermalZone>();
  EXPECT_EQ(thermalZones1.size(), thermalZones2.size());

  for (auto& thermalZone : thermalZones1) {
    auto match = baseline.getModelObjectByName<ThermalZone>(*thermalZone.name());
    EXPECT_TRUE(match.has_value());
    // Multiplier
    EXPECT_EQ(thermalZone.multiplier(), match->multiplier());

    // Compare primary and secondary daylighting
    // Note names don't matchm the floorspace translator is Daylighting Control 1 - 1
    // as in the original floorspace model but the threeJS rtranslatot is Face NN
    auto primaryDaylighting1 = thermalZone.primaryDaylightingControl();
    auto primaryDaylighting2 = match->primaryDaylightingControl();
    EXPECT_EQ(primaryDaylighting1.has_value(), primaryDaylighting2.has_value());
    //if (primaryDaylighting1.has_value()) {
    //  ASSERT_EQ(*primaryDaylighting1->name(), *primaryDaylighting2->name());
    //}
    auto secondaryDaylighting1 = thermalZone.primaryDaylightingControl();
    auto secondaryDaylighting2 = match->primaryDaylightingControl();
    EXPECT_EQ(secondaryDaylighting1.has_value(), secondaryDaylighting2.has_value());
    //if (secondaryDaylighting1.has_value()) {
    //  ASSERT_EQ(*secondaryDaylighting1->name(), *secondaryyDaylighting2->name());
    //}
    if (primaryDaylighting1.has_value()) {
      EXPECT_EQ(primaryDaylighting1->illuminanceSetpoint(), primaryDaylighting2->illuminanceSetpoint());
      EXPECT_EQ(primaryDaylighting1->position(), primaryDaylighting2->position());
    }

    if (secondaryDaylighting1.has_value()) {
      EXPECT_EQ(secondaryDaylighting1->illuminanceSetpoint(), secondaryDaylighting2->illuminanceSetpoint());
      EXPECT_EQ(secondaryDaylighting1->position(), secondaryDaylighting2->position());
    }

    auto groupRendering1 = thermalZone.renderingColor();
    auto groupRendering2 = match->renderingColor();
    EXPECT_EQ(groupRendering1.has_value(), groupRendering2.has_value());
  }

  // Compare building units
  auto buildingUnits1 = model.getModelObjects<BuildingUnit>();
  auto buildingUnits2 = baseline.getModelObjects<BuildingUnit>();
  EXPECT_EQ(buildingUnits1.size(), buildingUnits2.size());
  for (auto& buildingUnit : buildingUnits1) {
    auto match = baseline.getModelObjectByName<BuildingUnit>(*buildingUnit.name());
    EXPECT_TRUE(match.has_value());
    EXPECT_EQ(buildingUnit.renderingColor().has_value(), match->renderingColor().has_value());
  }

  // Compare constructions
  auto constructions1 = model.getModelObjects<DefaultConstructionSet>();
  auto constructions2 = baseline.getModelObjects<DefaultConstructionSet>();
  EXPECT_EQ(constructions1.size(), constructions2.size());
  for (auto& construction : constructions1) {
    auto match = baseline.getModelObjectByName<DefaultConstructionSet>(*construction.name());
    EXPECT_TRUE(match.has_value());
  }

  // Compare daylighting
  auto daylightings1 = model.getModelObjects<DaylightingControl>();
  auto daylightings2 = baseline.getModelObjects<DaylightingControl>();
  EXPECT_EQ(daylightings1.size(), daylightings2.size());
  for (auto& daylighting : daylightings1) {
    auto match = baseline.getModelObjectByName<DaylightingControl>(*daylighting.name());
    // Daylighting control names don't match:
    // Daylighting Control 1 - 1 (as in the floorspace model) vs Face nn
    //ASSERT_TRUE(match.has_value());
  }

  // Compare ShadingSurfaceGroup
  auto shadingGroup1 = model.getModelObjects<ShadingSurfaceGroup>();
  auto shadingGroup2 = baseline.getModelObjects<ShadingSurfaceGroup>();
  EXPECT_EQ(shadingGroup1.size(), shadingGroup2.size());
  for (auto& shadingGroup : shadingGroup1) {
    // Shading Surface Group names only match for building shading
    if (shadingGroup.shadingSurfaceType() == "Building") {
      auto match = baseline.getModelObjectByName<ShadingSurfaceGroup>(*shadingGroup.name());
      EXPECT_TRUE(match.has_value());
      std::string surfaceType = shadingGroup.shadingSurfaceType();
      EXPECT_EQ(shadingGroup.shadingSurfaceType(), match->shadingSurfaceType());
      EXPECT_EQ(shadingGroup.shadingSurfaces().size(), match->shadingSurfaces().size());

      // Match surfaces by vertices
      for (auto& shadingSurface : shadingGroup.shadingSurfaces()) {
        bool matched = false;
        for (auto& shadingSurface2 : match->shadingSurfaces()) {
          if (circularEqual(shadingSurface.vertices(), shadingSurface2.vertices())) {
            matched = true;
            break;
          }
        }
        EXPECT_TRUE(matched);
      }
    }
  }

  // Compare stories
  auto stories1 = model.getConcreteModelObjects<BuildingStory>();
  auto stories2 = baseline.getConcreteModelObjects<BuildingStory>();
  EXPECT_EQ(stories1.size(), stories2.size());
  for (auto& story : stories1) {
    OptionalBuildingStory match = baseline.getConcreteModelObjectByName<BuildingStory>(*story.name());
    EXPECT_TRUE(match.has_value());
    EXPECT_NEAR(*story.nominalZCoordinate(), *match->nominalZCoordinate(), 0.001);
    EXPECT_NEAR(*story.nominalFloortoFloorHeight(), *match->nominalFloortoFloorHeight(), 0.001);
    EXPECT_NEAR(*story.nominalFloortoCeilingHeight(), *match->nominalFloortoCeilingHeight(), 0.001);
    EXPECT_EQ(story.defaultConstructionSet().has_value(), match->defaultConstructionSet().has_value());
    if (story.defaultConstructionSet().has_value()) {
      EXPECT_EQ(story.defaultConstructionSet()->name(), match->defaultConstructionSet()->name());
    }
  }

  // Compare spaces/surfaces and subsurfaces
  auto spaces1 = model.getConcreteModelObjects<Space>();
  auto spaces2 = baseline.getConcreteModelObjects<Space>();
  EXPECT_EQ(spaces1.size(), spaces2.size());
  for (auto& space : spaces1) {
    std::string name = *space.name();
    //ASSERT_EQ(model.getConcreteModelObjectsByName<Space>(*space.name()).size(), 1);
    OptionalSpace match = baseline.getConcreteModelObjectByName<Space>(*space.name());
    EXPECT_TRUE(match.has_value());
    // Check Space Type
    EXPECT_EQ(space.spaceType().has_value(), match->spaceType().has_value());
    if (space.spaceType().has_value()) {
      EXPECT_EQ(space.spaceType()->name(), match->spaceType()->name());
    }
    // Check Default Construction Set
    EXPECT_EQ(space.defaultConstructionSet().has_value(), match->defaultConstructionSet().has_value());
    if (space.defaultConstructionSet().has_value()) {
      EXPECT_EQ(space.defaultConstructionSet()->name(), match->defaultConstructionSet()->name());
    }
    // Check Building Story
    EXPECT_EQ(space.buildingStory().has_value(), match->buildingStory().has_value());
    if (space.buildingStory().has_value()) {
      EXPECT_EQ(space.buildingStory()->name(), match->buildingStory()->name());
    }
    // Check Thermal Zone
    EXPECT_EQ(space.thermalZone().has_value(), match->thermalZone().has_value());
    if (space.thermalZone().has_value()) {
      EXPECT_EQ(space.thermalZone()->name(), match->thermalZone()->name());
    }
    // Check Building Unit
    EXPECT_EQ(space.buildingUnit().has_value(), match->buildingUnit().has_value());
    // Multiplier
    EXPECT_EQ(space.multiplier(), match->multiplier());
    if (space.buildingUnit().has_value()) {
      EXPECT_EQ(space.buildingUnit()->name(), match->buildingUnit()->name());
    }
    // CHeck daylighting control
    EXPECT_EQ(space.daylightingControls().size(), match->daylightingControls().size());

    // Check the control themselves. Because daylightingcontrol have different names
    // Daylighting Control 1 - 1 (as in the floorspace model) vs Face nn) they cannot be
    // compared 1:1, but we can compare them via the space. They should be a match
    // primary vs primary, secondary vs secondary.
    // No ths doe snot work because the order they are returned in the list isnt deterministic
    //for (unsigned i = 0; i < space.daylightingControls().size(); i++) {
    //  ASSERT_EQ(space.daylightingControls()[i].illuminanceSetpoint(), match->daylightingControls()[i].illuminanceSetpoint());
    //  ASSERT_EQ(space.daylightingControls()[i].position(), match->daylightingControls()[i].position());
    //  ASSERT_EQ(space.daylightingControls()[i].isPsiRotationAroundXAxisDefaulted(),
    //            match->daylightingControls()[i].isPsiRotationAroundXAxisDefaulted());
    //}

    // Check space surfaces and subsurfaces
    EXPECT_EQ(space.surfaces().size(), match->surfaces().size());

    for (auto& surface1 : space.surfaces()) {
      std::string name1 = *surface1.name();
      bool matchedSurface = false;
      for (auto& surface2 : match->surfaces()) {
        if (surface1.surfaceType() != surface2.surfaceType()) continue;
        std::string name2 = *surface2.name();
        if (circularEqual(surface1.vertices(), surface2.vertices())) {
          if (getDistance(surface1.vertices()[0], surface2.vertices()[0]) < tol) {

            matchedSurface = true;
            EXPECT_EQ(surface1.surfaceType(), surface2.surfaceType());

            // Check the subsurfaces
            EXPECT_EQ(surface1.subSurfaces().size(), surface2.subSurfaces().size());

            bool matchedSubsurface = false;
            for (auto& subSurface1 : surface1.subSurfaces()) {
              for (auto& subSurface2 : surface2.subSurfaces()) {
                if (circularEqual(subSurface1.vertices(), subSurface2.vertices())) {
                  if (getDistance(subSurface1.vertices()[0], subSurface2.vertices()[0]) < tol) {

                    matchedSubsurface = true;
                    break;
                  }
                }
              }
              EXPECT_TRUE(matchedSubsurface);
            }
            break;
          }
        }
      }
      EXPECT_TRUE(matchedSurface);
    }
  }

  auto shading1 = model.getConcreteModelObjects<ShadingSurfaceGroup>();
  auto shading2 = baseline.getConcreteModelObjects<ShadingSurfaceGroup>();
  EXPECT_EQ(shading1.size(), shading2.size());
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Compare_SimpleTest) {

  ThreeJSReverseTranslator rt;

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/simple_test.json");
  ASSERT_TRUE(exists(p));

  boost::optional<FloorplanJS> floorPlan = FloorplanJS::load(toString(p));
  ASSERT_TRUE(floorPlan);

  // not triangulated, for model transport/translation
  ThreeScene scene = floorPlan->toThreeScene(true);

  boost::optional<Model> model = rt.modelFromThreeJS(scene);
  ASSERT_TRUE(model);
  model->save(resourcesPath() / toPath("utilities/Geometry/floorspaceviathreejs.osm"), true);

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model1 = frt.modelFromFloorspace(toString(p));
  ASSERT_TRUE(model1);

  model1->save(resourcesPath() / toPath("utilities/Geometry/floorspacedirect.osm"), true);

  CompareTwoModels(*model1, *model);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Compare_Office_Floorplan) {

  ThreeJSReverseTranslator rt;

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/office_floorplan.json");
  ASSERT_TRUE(exists(p));

  boost::optional<FloorplanJS> floorPlan = FloorplanJS::load(toString(p));
  ASSERT_TRUE(floorPlan);

  // not triangulated, for model transport/translation
  ThreeScene scene = floorPlan->toThreeScene(true);

  boost::optional<Model> model = rt.modelFromThreeJS(scene);
  ASSERT_TRUE(model);
  model->save(resourcesPath() / toPath("utilities/Geometry/floorspaceviathreejs.osm"), true);

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model1 = frt.modelFromFloorspace(toString(p));
  ASSERT_TRUE(model1);

  model1->save(resourcesPath() / toPath("utilities/Geometry/floorspacedirect.osm"), true);

  CompareTwoModels(*model1, *model);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS) {

  openstudio::path out;

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));
  ASSERT_TRUE(model);

  model->save(resourcesPath() / toPath("model/FloorplanToThreeJSNonTriangulatedToModel.osm"), true);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_SurfaceMatch) {

  FloorspaceReverseTranslator rt;

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/surface_match_floorplan.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));
  ASSERT_TRUE(model);

  model->save(resourcesPath() / toPath("model/surface_match_floorplan.osm"), true);

  std::stringstream ss;
  ss << *model;
  std::string s = ss.str();

  EXPECT_EQ(4u, model->getModelObjects<Space>().size());
  EXPECT_EQ(24u, model->getModelObjects<Surface>().size());

  unsigned numMatched = 0;
  for (const auto& surface : model->getModelObjects<Surface>()) {
    if (surface.outsideBoundaryCondition() == "Surface") {
      EXPECT_TRUE(surface.adjacentSurface());
      ++numMatched;
    }
  }
  EXPECT_EQ(8u, numMatched);

  // merge export model into newModel
  model::Model newModel;
  model::ModelMerger mm;
  mm.mergeModels(newModel, *model, rt.handleMapping());

  EXPECT_EQ(4u, newModel.getModelObjects<Space>().size());
  EXPECT_EQ(24u, newModel.getModelObjects<Surface>().size());

  numMatched = 0;
  for (const auto& surface : newModel.getModelObjects<Surface>()) {
    if (surface.outsideBoundaryCondition() == "Surface") {
      EXPECT_TRUE(surface.adjacentSurface());
      ++numMatched;
    }
  }
  EXPECT_EQ(8u, numMatched);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Doors) {

  ThreeJSReverseTranslator rt;

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan_doors.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));
  ASSERT_TRUE(model);

  model->save(resourcesPath() / toPath("model/floorplan_doors.osm"), true);

  ThreeJSForwardTranslator ft;
  ft.modelToThreeJS(*model, true);

  ASSERT_EQ(1u, model->getConcreteModelObjects<Space>().size());

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Space 1 - 1");
  ASSERT_TRUE(space1);

  unsigned numDoors = 0;
  unsigned numGlassDoors = 0;
  unsigned numOverheadDoors = 0;
  double doorArea = 0;
  double glassDoorArea = 0;
  double overheadDoorArea = 0;

  for (const auto& surface : space1->surfaces()) {
    for (const auto& subSurface : surface.subSurfaces()) {
      if (subSurface.subSurfaceType() == "Door") {
        doorArea += subSurface.grossArea();
        numDoors++;
      } else if (subSurface.subSurfaceType() == "GlassDoor") {
        glassDoorArea += subSurface.grossArea();
        numGlassDoors++;
      } else if (subSurface.subSurfaceType() == "OverheadDoor") {
        overheadDoorArea += subSurface.grossArea();
        numOverheadDoors++;
      }
    }
  }

  EXPECT_EQ(3, numDoors);
  EXPECT_EQ(3, numGlassDoors);
  EXPECT_EQ(2, numOverheadDoors);

  EXPECT_NEAR(convert(3.0 * 3.0 * 6.67, "ft^2", "m^2").get(), doorArea, 0.01);
  EXPECT_NEAR(convert(3.0 * 6.0 * 6.67, "ft^2", "m^2").get(), glassDoorArea, 0.01);
  EXPECT_NEAR(convert(2.0 * 15.0 * 8.0, "ft^2", "m^2").get(), overheadDoorArea, 0.01);

  Model model2;
  ModelMerger merger;
  std::map<UUID, UUID> handleMapping;
  merger.mergeModels(model2, *model, handleMapping);

  ASSERT_EQ(1u, model->getConcreteModelObjects<Space>().size());

  space1 = model2.getConcreteModelObjectByName<Space>("Space 1 - 1");
  ASSERT_TRUE(space1);

  numDoors = 0;
  numGlassDoors = 0;
  numOverheadDoors = 0;
  doorArea = 0;
  glassDoorArea = 0;
  overheadDoorArea = 0;

  for (const auto& surface : space1->surfaces()) {
    for (const auto& subSurface : surface.subSurfaces()) {
      if (subSurface.subSurfaceType() == "Door") {
        doorArea += subSurface.grossArea();
        numDoors++;
      } else if (subSurface.subSurfaceType() == "GlassDoor") {
        glassDoorArea += subSurface.grossArea();
        numGlassDoors++;
      } else if (subSurface.subSurfaceType() == "OverheadDoor") {
        overheadDoorArea += subSurface.grossArea();
        numOverheadDoors++;
      }
    }
  }

  EXPECT_EQ(3, numDoors);
  EXPECT_EQ(3, numGlassDoors);
  EXPECT_EQ(2, numOverheadDoors);

  EXPECT_NEAR(convert(3.0 * 3.0 * 6.67, "ft^2", "m^2").get(), doorArea, 0.01);
  EXPECT_NEAR(convert(3.0 * 6.0 * 6.67, "ft^2", "m^2").get(), glassDoorArea, 0.01);
  EXPECT_NEAR(convert(2.0 * 15.0 * 8.0, "ft^2", "m^2").get(), overheadDoorArea, 0.01);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Windows) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/window_floorplan.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));

  model->save(resourcesPath() / toPath("model/floorplan_windows.osm"), true);

  std::stringstream ss;
  ss << *model;
  std::string s = ss.str();

  ThreeJSForwardTranslator ft;
  ft.modelToThreeJS(*model, true);

  ASSERT_EQ(3u, model->getConcreteModelObjects<Space>().size());

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Space 1");
  boost::optional<Space> space2 = model->getConcreteModelObjectByName<Space>("Space 2");
  boost::optional<Space> space3 = model->getConcreteModelObjectByName<Space>("Space 3");
  ASSERT_TRUE(space1);
  ASSERT_TRUE(space2);
  ASSERT_TRUE(space3);

  std::vector<Space> spaces;
  spaces.push_back(*space1);
  spaces.push_back(*space2);
  spaces.push_back(*space3);

  struct SpaceInfo
  {
    std::string name;
    unsigned numExteriorWalls;
    boost::optional<Surface> southSurface;
    std::vector<SubSurface> windows;
    std::vector<DaylightingControl> dcs;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    info.numExteriorWalls = 0;
    for (const auto& surface : space.surfaces()) {
      if (surface.surfaceType() == "Wall") {
        if (surface.outsideBoundaryCondition() == "Outdoors") {
          info.numExteriorWalls += 1;
          if ((surface.azimuth() > degToRad(179)) && (surface.azimuth() < degToRad(181))) {
            // only one
            EXPECT_FALSE(info.southSurface) << info.name;
            info.southSurface = surface;
            for (const auto& subSurface : surface.subSurfaces()) {
              info.windows.push_back(subSurface);
            }
          }
        }
      }
    }

    for (const auto& dc : space.daylightingControls()) {
      info.dcs.push_back(dc);
    }

    infos.push_back(info);
  }

  std::sort(std::begin(infos), std::end(infos), [](SpaceInfo a, SpaceInfo b) { return a.name < b.name; });

  ASSERT_EQ(3u, infos.size());

  EXPECT_EQ("Space 1", infos[0].name);
  EXPECT_EQ(3, infos[0].numExteriorWalls);
  ASSERT_TRUE(infos[0].southSurface);
  EXPECT_EQ(9, infos[0].windows.size());
  for (const auto& window : infos[0].windows) {
    EXPECT_NEAR(convert(8.0, "ft^2", "m^2").get(), window.grossArea(), 0.001);
    ASSERT_EQ(1u, window.shadingSurfaceGroups().size());
    EXPECT_EQ(3u, window.shadingSurfaceGroups()[0].shadingSurfaces().size());  // overhang + fins
  }
  EXPECT_NEAR(convert(800.0, "ft^2", "m^2").get(), infos[0].southSurface->grossArea(), 0.001);
  EXPECT_NEAR(convert(800.0 - 9 * 8.0, "ft^2", "m^2").get(), infos[0].southSurface->netArea(), 0.001);
  EXPECT_EQ(1, infos[0].dcs.size());

  EXPECT_EQ("Space 2", infos[1].name);
  EXPECT_EQ(2, infos[1].numExteriorWalls);
  ASSERT_TRUE(infos[1].southSurface);
  EXPECT_EQ(17, infos[1].windows.size());
  for (const auto& window : infos[1].windows) {
    EXPECT_NEAR(convert(8.0, "ft^2", "m^2").get(), window.grossArea(), 0.001);
    EXPECT_EQ(0, window.shadingSurfaceGroups().size());  // no shades
  }
  EXPECT_NEAR(convert(800.0, "ft^2", "m^2").get(), infos[1].southSurface->grossArea(), 0.001);
  EXPECT_NEAR(convert(800.0 - 17 * 8.0, "ft^2", "m^2").get(), infos[1].southSurface->netArea(), 0.001);
  EXPECT_EQ(1, infos[1].dcs.size());

  EXPECT_EQ("Space 3", infos[2].name);
  EXPECT_EQ(3, infos[2].numExteriorWalls);
  ASSERT_TRUE(infos[2].southSurface);
  EXPECT_EQ(1, infos[2].windows.size());
  for (const auto& window : infos[2].windows) {
    EXPECT_NEAR(convert(0.4 * 800.0, "ft^2", "m^2").get(), window.grossArea(), 0.001);
    ASSERT_EQ(1u, window.shadingSurfaceGroups().size());
    EXPECT_EQ(1u, window.shadingSurfaceGroups()[0].shadingSurfaces().size());  // just overhang
  }
  EXPECT_NEAR(convert(800.0, "ft^2", "m^2").get(), infos[2].southSurface->grossArea(), 0.001);
  EXPECT_NEAR(convert(0.6 * 800.0, "ft^2", "m^2").get(), infos[2].southSurface->netArea(), 0.001);
  EXPECT_EQ(1, infos[2].dcs.size());
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_SimAUD_Paper) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan-paper.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/floorplan-paper.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Shading) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan_shading.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/floorplan_shading.osm");
  model->save(outpath, true);

  ASSERT_EQ(1u, model->getConcreteModelObjects<Space>().size());
  ASSERT_EQ(2u, model->getConcreteModelObjects<ShadingSurfaceGroup>().size());

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Space 1 - 1");
  ASSERT_TRUE(space1);

  boost::optional<ShadingSurfaceGroup> shading1 = model->getConcreteModelObjectByName<ShadingSurfaceGroup>("Shading 1 - 1");
  boost::optional<ShadingSurfaceGroup> shading2 = model->getConcreteModelObjectByName<ShadingSurfaceGroup>("Shading 1 - 2");
  ASSERT_TRUE(shading1);
  ASSERT_TRUE(shading2);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_SplitLevel) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/split_level.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/split_level.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  EXPECT_EQ(2u, model->getConcreteModelObjects<BuildingStory>().size());
  EXPECT_EQ(3u, model->getConcreteModelObjects<Space>().size());

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Main Space");
  boost::optional<Space> space2 = model->getConcreteModelObjectByName<Space>("Split Level");
  boost::optional<Space> space3 = model->getConcreteModelObjectByName<Space>("Upper floor");
  ASSERT_TRUE(space1);
  ASSERT_TRUE(space2);
  ASSERT_TRUE(space3);

  ASSERT_TRUE(space1->buildingStory());
  EXPECT_EQ("Story 1", space1->buildingStory()->nameString());
  ASSERT_TRUE(space2->buildingStory());
  EXPECT_EQ("Story 1", space2->buildingStory()->nameString());
  ASSERT_TRUE(space3->buildingStory());
  EXPECT_EQ("Story 2", space3->buildingStory()->nameString());

  std::vector<Space> spaces;
  spaces.push_back(*space1);
  spaces.push_back(*space2);
  spaces.push_back(*space3);

  struct SpaceInfo
  {
    std::string name;
    unsigned numExteriorWalls;
    unsigned numInteriorWalls;
    double exteriorWallArea;
    double interiorWallArea;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    info.numExteriorWalls = 0;
    info.numInteriorWalls = 0;
    info.exteriorWallArea = 0;
    info.interiorWallArea = 0;
    for (const auto& surface : space.surfaces()) {
      if (surface.surfaceType() == "Wall") {
        if (surface.outsideBoundaryCondition() == "Outdoors") {
          info.numExteriorWalls += 1;
          info.exteriorWallArea += surface.grossArea();
        } else if (surface.outsideBoundaryCondition() == "Surface") {
          info.numInteriorWalls += 1;
          info.interiorWallArea += surface.grossArea();
        }
      }
    }
    infos.push_back(info);
  }

  ASSERT_EQ(3u, infos.size());

  EXPECT_EQ("Main Space", infos[0].name);
  EXPECT_EQ(4, infos[0].numExteriorWalls);
  EXPECT_EQ(1, infos[0].numInteriorWalls);
  EXPECT_NEAR(convert(3 * 800.0 + 400.0, "ft^2", "m^2").get(), infos[0].exteriorWallArea, 0.001);
  EXPECT_NEAR(convert(400.0, "ft^2", "m^2").get(), infos[0].interiorWallArea, 0.001);

  EXPECT_EQ("Split Level", infos[1].name);
  EXPECT_EQ(3, infos[1].numExteriorWalls);
  EXPECT_EQ(2, infos[1].numInteriorWalls);
  EXPECT_NEAR(convert(3 * 800.0, "ft^2", "m^2").get(), infos[1].exteriorWallArea, 0.001);
  EXPECT_NEAR(convert(2 * 400.0, "ft^2", "m^2").get(), infos[1].interiorWallArea, 0.001);

  EXPECT_EQ("Upper floor", infos[2].name);
  EXPECT_EQ(4, infos[2].numExteriorWalls);
  EXPECT_EQ(1, infos[2].numInteriorWalls);
  EXPECT_NEAR(convert(3 * 800.0 + 400.0, "ft^2", "m^2").get(), infos[2].exteriorWallArea, 0.001);
  EXPECT_NEAR(convert(400.0, "ft^2", "m^2").get(), infos[2].interiorWallArea, 0.001);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_StorySpaceHeights) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/story_space_heights.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/story_space_heights.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  EXPECT_EQ(1u, model->getConcreteModelObjects<BuildingStory>().size());
  EXPECT_EQ(4u, model->getConcreteModelObjects<Space>().size());
  EXPECT_EQ(4u, model->getConcreteModelObjects<ThermalZone>().size());

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Story Default");
  boost::optional<Space> space2 = model->getConcreteModelObjectByName<Space>("Story Default Plenum");
  boost::optional<Space> space3 = model->getConcreteModelObjectByName<Space>("Story Default Floor Plenum");
  boost::optional<Space> space4 = model->getConcreteModelObjectByName<Space>("Tall No Plenums");
  ASSERT_TRUE(space1);
  ASSERT_TRUE(space2);
  ASSERT_TRUE(space3);
  ASSERT_TRUE(space4);

  ASSERT_TRUE(space1->buildingStory());
  EXPECT_EQ("Story 1", space1->buildingStory()->nameString());
  ASSERT_TRUE(space2->buildingStory());
  EXPECT_EQ("Story 1", space2->buildingStory()->nameString());
  ASSERT_TRUE(space3->buildingStory());
  EXPECT_EQ("Story 1", space3->buildingStory()->nameString());
  ASSERT_TRUE(space4->buildingStory());
  EXPECT_EQ("Story 1", space4->buildingStory()->nameString());

  ASSERT_TRUE(space1->spaceType());
  EXPECT_EQ("Space Type 1", space1->spaceType()->nameString());
  ASSERT_TRUE(space2->spaceType());
  EXPECT_EQ("Plenum Space Type", space2->spaceType()->nameString());
  EXPECT_EQ(model->plenumSpaceType().handle(), space2->spaceType()->handle());
  ASSERT_TRUE(space3->spaceType());
  EXPECT_EQ("Plenum Space Type", space3->spaceType()->nameString());
  EXPECT_EQ(model->plenumSpaceType().handle(), space3->spaceType()->handle());
  ASSERT_TRUE(space4->spaceType());
  EXPECT_EQ("Space Type 1", space4->spaceType()->nameString());

  // plenums are assigned to unique zones, but the zones are not yet plenums as they are not connected to an air loop
  ASSERT_TRUE(space1->thermalZone());
  EXPECT_EQ("Thermal Zone 1", space1->thermalZone()->nameString());
  EXPECT_FALSE(space1->thermalZone()->isPlenum());
  ASSERT_TRUE(space2->thermalZone());
  EXPECT_EQ("Thermal Zone 1 Plenum", space2->thermalZone()->nameString());
  EXPECT_FALSE(space2->thermalZone()->isPlenum());
  ASSERT_TRUE(space3->buildingStory());
  EXPECT_EQ("Thermal Zone 1 Floor Plenum", space3->thermalZone()->nameString());
  EXPECT_FALSE(space3->thermalZone()->isPlenum());
  ASSERT_TRUE(space4->buildingStory());
  EXPECT_EQ("Thermal Zone 2", space4->thermalZone()->nameString());
  EXPECT_FALSE(space4->thermalZone()->isPlenum());

  std::vector<Space> spaces;
  spaces.push_back(*space1);
  spaces.push_back(*space2);
  spaces.push_back(*space3);
  spaces.push_back(*space4);

  double volume1 = 0;
  volume1 += space1->volume();
  volume1 += space2->volume();
  volume1 += space3->volume();

  EXPECT_NEAR(volume1, space4->volume(), 0.001);

  struct SpaceInfo
  {
    std::string name;
    unsigned numExterior;
    unsigned numInterior;
    double exteriorArea;
    double interiorArea;
    std::set<std::string> adjacentSpaces;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    info.numExterior = 0;
    info.numInterior = 0;
    info.exteriorArea = 0;
    info.interiorArea = 0;
    for (const auto& surface : space.surfaces()) {

      if ((surface.outsideBoundaryCondition() == "Outdoors") || (surface.outsideBoundaryCondition() == "Ground")) {
        info.numExterior += 1;
        info.exteriorArea += surface.grossArea();
      } else if (surface.outsideBoundaryCondition() == "Surface") {
        info.numInterior += 1;
        info.interiorArea += surface.grossArea();
        ASSERT_TRUE(surface.adjacentSurface());
        ASSERT_TRUE(surface.adjacentSurface()->space());
        info.adjacentSpaces.insert(surface.adjacentSurface()->space()->nameString());
      }
    }
    infos.push_back(info);
  }

  ASSERT_EQ(4u, infos.size());

  EXPECT_EQ("Story Default", infos[0].name);
  EXPECT_EQ(3, infos[0].numExterior);
  EXPECT_EQ(3, infos[0].numInterior);
  EXPECT_NEAR(convert(3 * 800.0, "ft^2", "m^2").get(), infos[0].exteriorArea, 0.001);
  EXPECT_NEAR(convert(2 * 100.0 * 100.0 + 800.0, "ft^2", "m^2").get(), infos[0].interiorArea, 0.001);
  EXPECT_EQ(3u, infos[0].adjacentSpaces.size());
  EXPECT_EQ(0u, infos[0].adjacentSpaces.count("Story Default"));
  EXPECT_EQ(1u, infos[0].adjacentSpaces.count("Story Default Plenum"));
  EXPECT_EQ(1u, infos[0].adjacentSpaces.count("Story Default Floor Plenum"));
  EXPECT_EQ(1u, infos[0].adjacentSpaces.count("Tall No Plenums"));

  EXPECT_EQ("Story Default Plenum", infos[1].name);
  EXPECT_EQ(4, infos[1].numExterior);
  EXPECT_EQ(2, infos[1].numInterior);
  EXPECT_NEAR(convert(3 * 200.0 + 100.0 * 100.0, "ft^2", "m^2").get(), infos[1].exteriorArea, 0.001);
  EXPECT_NEAR(convert(200.0 + 100.0 * 100.0, "ft^2", "m^2").get(), infos[1].interiorArea, 0.001);
  EXPECT_EQ(2u, infos[1].adjacentSpaces.size());
  EXPECT_EQ(1u, infos[1].adjacentSpaces.count("Story Default"));
  EXPECT_EQ(0u, infos[1].adjacentSpaces.count("Story Default Plenum"));
  EXPECT_EQ(0u, infos[1].adjacentSpaces.count("Story Default Floor Plenum"));
  EXPECT_EQ(1u, infos[1].adjacentSpaces.count("Tall No Plenums"));

  EXPECT_EQ("Story Default Floor Plenum", infos[2].name);
  EXPECT_EQ(4, infos[2].numExterior);
  EXPECT_EQ(2, infos[2].numInterior);
  EXPECT_NEAR(convert(3 * 200.0 + 100.0 * 100.0, "ft^2", "m^2").get(), infos[2].exteriorArea, 0.001);
  EXPECT_NEAR(convert(200.0 + 100.0 * 100.0, "ft^2", "m^2").get(), infos[2].interiorArea, 0.001);
  EXPECT_EQ(2u, infos[2].adjacentSpaces.size());
  EXPECT_EQ(1u, infos[2].adjacentSpaces.count("Story Default"));
  EXPECT_EQ(0u, infos[2].adjacentSpaces.count("Story Default Plenum"));
  EXPECT_EQ(0u, infos[2].adjacentSpaces.count("Story Default Floor Plenum"));
  EXPECT_EQ(1u, infos[2].adjacentSpaces.count("Tall No Plenums"));

  EXPECT_EQ("Tall No Plenums", infos[3].name);
  EXPECT_EQ(5, infos[3].numExterior);
  EXPECT_EQ(3, infos[3].numInterior);
  EXPECT_NEAR(convert(3 * 1200.0 + 2 * 100.0 * 100.0, "ft^2", "m^2").get(), infos[3].exteriorArea, 0.001);
  EXPECT_NEAR(convert(1200.0, "ft^2", "m^2").get(), infos[3].interiorArea, 0.001);
  EXPECT_EQ(3u, infos[3].adjacentSpaces.size());
  EXPECT_EQ(1u, infos[3].adjacentSpaces.count("Story Default"));
  EXPECT_EQ(1u, infos[3].adjacentSpaces.count("Story Default Plenum"));
  EXPECT_EQ(1u, infos[3].adjacentSpaces.count("Story Default Floor Plenum"));
  EXPECT_EQ(0u, infos[3].adjacentSpaces.count("Tall No Plenums"));
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Colors) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan_colors.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/floorplan_colors.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0u, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  boost::optional<BuildingStory> story = model->getConcreteModelObjectByName<BuildingStory>("Black Story");
  boost::optional<Space> space = model->getConcreteModelObjectByName<Space>("White Space");
  boost::optional<ThermalZone> thermalZone = model->getConcreteModelObjectByName<ThermalZone>("Red Zone");
  boost::optional<SpaceType> spaceType = model->getConcreteModelObjectByName<SpaceType>("Green Type");
  boost::optional<DefaultConstructionSet> constructionSet = model->getConcreteModelObjectByName<DefaultConstructionSet>("Blue Set");
  ASSERT_TRUE(story);
  ASSERT_TRUE(space);
  ASSERT_TRUE(thermalZone);
  ASSERT_TRUE(spaceType);
  ASSERT_TRUE(constructionSet);

  ASSERT_TRUE(story->renderingColor());
  EXPECT_EQ(0, story->renderingColor()->renderingRedValue());
  EXPECT_EQ(0, story->renderingColor()->renderingGreenValue());
  EXPECT_EQ(0, story->renderingColor()->renderingBlueValue());
  EXPECT_EQ(255, story->renderingColor()->renderingAlphaValue());

  //ASSERT_TRUE(space->renderingColor());
  //EXPECT_EQ(255, space->renderingColor()->renderingRedValue());
  //EXPECT_EQ(255, space->renderingColor()->renderingGreenValue());
  //EXPECT_EQ(255, space->renderingColor()->renderingBlueValue());
  //EXPECT_EQ(255, space->renderingColor()->renderingAlphaValue());

  ASSERT_TRUE(thermalZone->renderingColor());
  EXPECT_EQ(255, thermalZone->renderingColor()->renderingRedValue());
  EXPECT_EQ(0, thermalZone->renderingColor()->renderingGreenValue());
  EXPECT_EQ(0, thermalZone->renderingColor()->renderingBlueValue());
  EXPECT_EQ(255, thermalZone->renderingColor()->renderingAlphaValue());

  ASSERT_TRUE(spaceType->renderingColor());
  EXPECT_EQ(0, spaceType->renderingColor()->renderingRedValue());
  EXPECT_EQ(255, spaceType->renderingColor()->renderingGreenValue());
  EXPECT_EQ(0, spaceType->renderingColor()->renderingBlueValue());
  EXPECT_EQ(255, spaceType->renderingColor()->renderingAlphaValue());

  //ASSERT_TRUE(constructionSet->renderingColor());
  //EXPECT_EQ(0, story->renderingColor()->renderingRedValue());
  //EXPECT_EQ(0, story->renderingColor()->renderingGreenValue());
  //EXPECT_EQ(255, story->renderingColor()->renderingBlueValue());
  //EXPECT_EQ(255, story->renderingColor()->renderingAlphaValue());
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_DifferingNumVertices) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan_differing_num_vertices.json");
  ASSERT_TRUE(exists(p));

  boost::optional<FloorplanJS> floorPlan = FloorplanJS::load(toString(p));
  ASSERT_TRUE(floorPlan);

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/floorplan_differing_num_vertices.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0u, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Space 1 - 1");
  boost::optional<Space> space2 = model->getConcreteModelObjectByName<Space>("Space 2 - 1");
  ASSERT_TRUE(space1);
  ASSERT_TRUE(space2);

  std::vector<Space> spaces = model->getConcreteModelObjects<Space>();

  struct SpaceInfo
  {
    std::string name;
    boost::optional<Surface> floor;
    boost::optional<Surface> ceiling;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    for (const auto& surface : space.surfaces()) {
      if (surface.surfaceType() == "Floor") {
        EXPECT_FALSE(info.floor);
        info.floor = surface;
      } else if (surface.surfaceType() == "RoofCeiling") {
        EXPECT_FALSE(info.ceiling);
        info.ceiling = surface;
      }
    }
    infos.push_back(info);
  }

  std::sort(std::begin(infos), std::end(infos), [](SpaceInfo a, SpaceInfo b) { return a.name < b.name; });

  ASSERT_EQ(2u, infos.size());

  EXPECT_EQ("Space 1 - 1", infos[0].name);
  ASSERT_TRUE(infos[0].floor);
  EXPECT_EQ(6u, infos[0].floor->vertices().size());
  ASSERT_TRUE(infos[0].ceiling);
  EXPECT_EQ(6u, infos[0].ceiling->vertices().size());
  EXPECT_FALSE(infos[0].floor->adjacentSurface());
  ASSERT_TRUE(infos[0].ceiling->adjacentSurface());
  ASSERT_TRUE(infos[0].ceiling->adjacentSurface()->space());
  EXPECT_EQ("Space 2 - 1", infos[0].ceiling->adjacentSurface()->space()->nameString());

  EXPECT_EQ("Space 2 - 1", infos[1].name);
  ASSERT_TRUE(infos[1].floor);
  EXPECT_EQ(4u, infos[1].floor->vertices().size());
  ASSERT_TRUE(infos[1].ceiling);
  EXPECT_EQ(4u, infos[1].ceiling->vertices().size());
  EXPECT_FALSE(infos[1].ceiling->adjacentSurface());
  ASSERT_TRUE(infos[1].floor->adjacentSurface());
  ASSERT_TRUE(infos[1].floor->adjacentSurface()->space());
  EXPECT_EQ("Space 1 - 1", infos[1].floor->adjacentSurface()->space()->nameString());
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_School) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan_school.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  auto handleMapping = rt.handleMapping();
  EXPECT_EQ(46, handleMapping.size());

  openstudio::path outpath = resourcesPath() / toPath("model/floorplan_school.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0u, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  std::vector<Space> spaces = model->getConcreteModelObjects<Space>();

  struct SpaceInfo
  {
    std::string name;
    std::vector<Surface> floors;
    std::vector<Surface> walls;
    std::vector<Surface> ceilings;
    unsigned numExteriorWalls = 0;
    unsigned numInteriorWalls = 0;
    double exteriorFloorArea = 0;
    double exteriorRoofArea = 0;
    double exteriorWallArea = 0;
    double interiorFloorArea = 0;
    double interiorRoofArea = 0;
    double interiorWallArea = 0;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    for (const auto& surface : space.surfaces()) {
      if (surface.surfaceType() == "Floor") {
        info.floors.push_back(surface);
        if (surface.outsideBoundaryCondition() == "Ground") {
          info.exteriorFloorArea += surface.grossArea();
        } else if (surface.outsideBoundaryCondition() == "Surface") {
          info.interiorFloorArea += surface.grossArea();
        }
      } else if (surface.surfaceType() == "RoofCeiling") {
        info.ceilings.push_back(surface);
        if (surface.outsideBoundaryCondition() == "Outdoors") {
          info.exteriorRoofArea += surface.grossArea();
        } else if (surface.outsideBoundaryCondition() == "Surface") {
          info.interiorRoofArea += surface.grossArea();
        }
      } else if (surface.surfaceType() == "Wall") {
        info.walls.push_back(surface);
        if (surface.outsideBoundaryCondition() == "Outdoors") {
          info.numExteriorWalls += 1;
          info.exteriorWallArea += surface.grossArea();
        } else if (surface.outsideBoundaryCondition() == "Surface") {
          info.numInteriorWalls += 1;
          info.interiorWallArea += surface.grossArea();
        }
      }
    }
    infos.push_back(info);
  }

  std::sort(std::begin(infos), std::end(infos), [](SpaceInfo a, SpaceInfo b) { return a.name < b.name; });

  ASSERT_EQ(33u, infos.size());

  auto it = std::find_if(infos.begin(), infos.end(), [](SpaceInfo a) { return a.name == "Space 2 - 1"; });
  ASSERT_TRUE(it != infos.end());
  EXPECT_EQ(1u, it->floors.size());
  EXPECT_NEAR(0.0, it->exteriorFloorArea, 1.0);
  EXPECT_NEAR(5796.0, it->interiorFloorArea, 1.0);
  EXPECT_EQ(1u, it->ceilings.size());
  EXPECT_NEAR(5796.0, it->exteriorRoofArea, 1.0);
  EXPECT_NEAR(0.0, it->interiorRoofArea, 1.0);
  EXPECT_EQ(9u, it->walls.size());
  EXPECT_EQ(2u, it->numExteriorWalls);
  EXPECT_EQ(7u, it->numInteriorWalls);
  EXPECT_NEAR(2178.75, it->exteriorWallArea, 1.0);
  EXPECT_NEAR(2546.25, it->interiorWallArea, 1.0);

  it = std::find_if(infos.begin(), infos.end(), [](SpaceInfo a) { return a.name == "Lobby 113"; });
  ASSERT_TRUE(it != infos.end());
  EXPECT_EQ(1u, it->floors.size());
  EXPECT_NEAR(1860.0, it->exteriorFloorArea, 1.0);
  EXPECT_NEAR(0.0, it->interiorFloorArea, 1.0);
  EXPECT_EQ(2u, it->ceilings.size());
  EXPECT_NEAR(600.0, it->exteriorRoofArea, 1.0);
  EXPECT_NEAR(1260.0, it->interiorRoofArea, 1.0);
  EXPECT_EQ(5u, it->walls.size());
  EXPECT_EQ(1u, it->numExteriorWalls);
  EXPECT_EQ(4u, it->numInteriorWalls);
  EXPECT_NEAR(813.75, it->exteriorWallArea, 1.0);
  EXPECT_NEAR(1601.25, it->interiorWallArea, 1.0);
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_StoryMultipliers) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/story_multipliers.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/story_multipliers.osm");
  model->save(outpath, true);

  // TODO: errors & warnings
  //EXPECT_EQ(0, rt.errors().size());
  //EXPECT_EQ(2u, rt.warnings().size());  // DLM: temporarily expect 2 warnings about non-translated multipliers

  //for (const auto& error : rt.errors()) {
  //  EXPECT_TRUE(false) << "Error: " << error.logMessage();
  //}

  //for (const auto& warning : rt.warnings()){
  //  EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  //}
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_StoryMultipliers2) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/story_multipliers2.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> model = frt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/story_multipliers2.osm");
  model->save(outpath, true);

  // TODO: errors & warnings
  //EXPECT_EQ(0, rt.errors().size());
  //EXPECT_EQ(1u, rt.warnings().size());  // DLM: temporarily expect 1 warnings about non-translated multipliers

  //for (const auto& error : rt.errors()) {
  //  EXPECT_TRUE(false) << "Error: " << error.logMessage();
  //}

  //for (const auto& warning : rt.warnings()){
  //  EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  //}
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_OpenToBelow) {

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/open_to_below.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator rt;
  boost::optional<Model> model = rt.modelFromFloorspace(toString(p));

  openstudio::path outpath = resourcesPath() / toPath("model/open_to_below.osm");
  model->save(outpath, true);

  EXPECT_EQ(0, rt.errors().size());
  EXPECT_EQ(0, rt.warnings().size());

  for (const auto& error : rt.errors()) {
    EXPECT_TRUE(false) << "Error: " << error.logMessage();
  }

  for (const auto& warning : rt.warnings()) {
    EXPECT_TRUE(false) << "Warning: " << warning.logMessage();
  }

  boost::optional<Space> space1 = model->getConcreteModelObjectByName<Space>("Open 1");
  boost::optional<Space> space2 = model->getConcreteModelObjectByName<Space>("Closed 1");
  boost::optional<Space> space3 = model->getConcreteModelObjectByName<Space>("Open 2");
  boost::optional<Space> space4 = model->getConcreteModelObjectByName<Space>("Closed 2");
  ASSERT_TRUE(space1);
  ASSERT_TRUE(space2);
  ASSERT_TRUE(space3);
  ASSERT_TRUE(space4);

  std::vector<Space> spaces;
  spaces.push_back(*space1);
  spaces.push_back(*space2);
  spaces.push_back(*space3);
  spaces.push_back(*space4);

  EXPECT_NEAR(convert(100.0 * 100.0, "ft^2", "m^2").get(), space1->floorArea(), 0.001);
  EXPECT_NEAR(convert(100.0 * 100.0, "ft^2", "m^2").get(), space2->floorArea(), 0.001);
  EXPECT_NEAR(convert(0.0, "ft^2", "m^2").get(), space3->floorArea(), 0.001);  // air wall floor
  EXPECT_NEAR(convert(100.0 * 100.0, "ft^2", "m^2").get(), space4->floorArea(), 0.001);

  struct SpaceInfo
  {
    std::string name;
    boost::optional<Surface> floor;
    boost::optional<Surface> ceiling;
  };

  std::vector<SpaceInfo> infos;
  for (const auto& space : spaces) {
    SpaceInfo info;
    info.name = space.nameString();
    for (const auto& surface : space.surfaces()) {
      if (surface.surfaceType() == "Floor") {
        EXPECT_FALSE(info.floor);
        info.floor = surface;
      } else if (surface.surfaceType() == "RoofCeiling") {
        EXPECT_FALSE(info.ceiling);
        info.ceiling = surface;
      }
    }
    infos.push_back(info);
  }

  ASSERT_EQ(4u, infos.size());

  EXPECT_EQ("Open 1", infos[0].name);
  ASSERT_TRUE(infos[0].floor);
  EXPECT_FALSE(infos[0].floor->isAirWall());
  EXPECT_FALSE(infos[0].floor->adjacentSurface());
  ASSERT_TRUE(infos[0].ceiling);
  EXPECT_TRUE(infos[0].ceiling->isAirWall());
  ASSERT_TRUE(infos[0].ceiling->adjacentSurface());
  ASSERT_TRUE(infos[0].ceiling->adjacentSurface()->space());
  EXPECT_EQ("Open 2", infos[0].ceiling->adjacentSurface()->space()->nameString());

  EXPECT_EQ("Closed 1", infos[1].name);
  ASSERT_TRUE(infos[1].floor);
  EXPECT_FALSE(infos[1].floor->isAirWall());
  EXPECT_FALSE(infos[1].floor->adjacentSurface());
  ASSERT_TRUE(infos[1].ceiling);
  EXPECT_FALSE(infos[1].ceiling->isAirWall());
  ASSERT_TRUE(infos[1].ceiling->adjacentSurface());
  ASSERT_TRUE(infos[1].ceiling->adjacentSurface()->space());
  EXPECT_EQ("Closed 2", infos[1].ceiling->adjacentSurface()->space()->nameString());

  EXPECT_EQ("Open 2", infos[2].name);
  ASSERT_TRUE(infos[2].floor);
  EXPECT_TRUE(infos[2].floor->isAirWall());
  ASSERT_TRUE(infos[2].floor->adjacentSurface());
  ASSERT_TRUE(infos[2].floor->adjacentSurface()->space());
  EXPECT_EQ("Open 1", infos[2].floor->adjacentSurface()->space()->nameString());
  ASSERT_TRUE(infos[2].ceiling);
  EXPECT_FALSE(infos[2].ceiling->isAirWall());
  EXPECT_FALSE(infos[2].ceiling->adjacentSurface());

  EXPECT_EQ("Closed 2", infos[3].name);
  ASSERT_TRUE(infos[3].floor);
  EXPECT_FALSE(infos[3].floor->isAirWall());
  ASSERT_TRUE(infos[3].floor->adjacentSurface());
  ASSERT_TRUE(infos[3].floor->adjacentSurface()->space());
  EXPECT_EQ("Closed 1", infos[3].floor->adjacentSurface()->space()->nameString());
  ASSERT_TRUE(infos[3].ceiling);
  EXPECT_FALSE(infos[3].ceiling->isAirWall());
  EXPECT_FALSE(infos[3].ceiling->adjacentSurface());
}

TEST_F(ModelFixture, FloorspaceReverseTranslator_FloorplanJS_Site_ClimateZones_4166) {

  // Test for #4166 - Merging FloorSpaceJS strips out climate zone assignment.

  // Start with a base model, put some climate zone in there
  // Add a Climate Zone to model1 only
  Model model;
  ClimateZones czs = model.getUniqueModelObject<ClimateZones>();
  ClimateZone cz = czs.setClimateZone(ClimateZones::ashraeInstitutionName(), "4A");
  EXPECT_EQ(1, czs.numClimateZones());
  EXPECT_EQ(1, czs.climateZones().size());

  // To reproduce the original issue, we also need a Site object since it's the fact that the Site object is deleted, and along with it its children
  // and that includes the ClimateZones
  Site site = model.getUniqueModelObject<Site>();
  ASSERT_EQ(1, site.children().size());
  EXPECT_EQ(czs, site.children().front());

  // I'm going to instantiate the Building object as well, to check if the floorplan.json north_axis (30) is properly written anyways
  Building building = model.getUniqueModelObject<Building>();
  EXPECT_TRUE(building.setNominalFloortoFloorHeight(2.5));

  // Now RT (any) floor plan back to a model

  openstudio::path p = resourcesPath() / toPath("utilities/Geometry/floorplan.json");
  ASSERT_TRUE(exists(p));

  FloorspaceReverseTranslator frt;
  boost::optional<Model> newModel_ = frt.modelFromFloorspace(toString(p));

  EXPECT_TRUE(model.getOptionalUniqueModelObject<ClimateZones>());
  EXPECT_FALSE(newModel_->getOptionalUniqueModelObject<ClimateZones>());

  EXPECT_EQ(0.0, building.northAxis());
  EXPECT_EQ(2.5, building.nominalFloortoFloorHeight());

  ASSERT_TRUE(newModel_->getOptionalUniqueModelObject<Building>());
  EXPECT_EQ(-30.0, newModel_->getOptionalUniqueModelObject<Building>()->northAxis());
  EXPECT_FALSE(newModel_->getOptionalUniqueModelObject<Building>()->nominalFloortoFloorHeight());

  model::ModelMerger mm;
  mm.mergeModels(model, newModel_.get(), frt.handleMapping());

  // Expect to still have retained the ClimateZone
  ASSERT_TRUE(model.getOptionalUniqueModelObject<ClimateZones>());
  EXPECT_EQ("4A", model.getOptionalUniqueModelObject<ClimateZones>()->climateZones()[0].value());
  EXPECT_EQ(ClimateZones::ashraeInstitutionName(), model.getOptionalUniqueModelObject<ClimateZones>()->climateZones()[0].institution());

  EXPECT_FALSE(newModel_->getOptionalUniqueModelObject<ClimateZones>());

  // It should have overridden only the things that were actually not defaulted, so building name and north axis
  ASSERT_TRUE(model.getOptionalUniqueModelObject<Building>());

  EXPECT_EQ(-30.0, model.getOptionalUniqueModelObject<Building>()->northAxis());
  ASSERT_TRUE(model.getOptionalUniqueModelObject<Building>()->nominalFloortoFloorHeight());
  EXPECT_EQ(2.5, model.getOptionalUniqueModelObject<Building>()->nominalFloortoFloorHeight());

  // New Model isn't touched anyways...
  ASSERT_TRUE(newModel_->getOptionalUniqueModelObject<Building>());
  EXPECT_EQ(-30.0, newModel_->getOptionalUniqueModelObject<Building>()->northAxis());
  EXPECT_FALSE(newModel_->getOptionalUniqueModelObject<Building>()->nominalFloortoFloorHeight());
}
