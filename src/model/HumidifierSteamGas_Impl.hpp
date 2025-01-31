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

#ifndef MODEL_HUMIDIFIERSTEAMGAS_IMPL_HPP
#define MODEL_HUMIDIFIERSTEAMGAS_IMPL_HPP

#include "ModelAPI.hpp"
#include "StraightComponent_Impl.hpp"

namespace openstudio {
namespace model {

  class Schedule;
  class Curve;

  namespace detail {

    /** HumidifierSteamGas_Impl is a StraightComponent_Impl that is the implementation class for HumidifierSteamGas.*/
    class MODEL_API HumidifierSteamGas_Impl : public StraightComponent_Impl
    {
     public:
      /** @name Constructors and Destructors */
      //@{

      HumidifierSteamGas_Impl(const IdfObject& idfObject, Model_Impl* model, bool keepHandle);

      HumidifierSteamGas_Impl(const openstudio::detail::WorkspaceObject_Impl& other, Model_Impl* model, bool keepHandle);

      HumidifierSteamGas_Impl(const HumidifierSteamGas_Impl& other, Model_Impl* model, bool keepHandle);

      virtual ~HumidifierSteamGas_Impl() {}

      //@}
      /** @name Virtual Methods */
      //@{

      virtual const std::vector<std::string>& outputVariableNames() const override;

      virtual IddObjectType iddObjectType() const override;

      virtual std::vector<ScheduleTypeKey> getScheduleTypeKeys(const Schedule& schedule) const override;

      virtual unsigned inletPort() const override;

      virtual unsigned outletPort() const override;

      //@}
      /** @name Getters */
      //@{

      boost::optional<Schedule> availabilitySchedule() const;

      boost::optional<double> ratedCapacity() const;

      bool isRatedCapacityAutosized() const;

      boost::optional<double> ratedGasUseRate() const;

      bool isRatedGasUseRateAutosized() const;

      double thermalEfficiency() const;

      bool isThermalEfficiencyDefaulted() const;

      boost::optional<Curve> thermalEfficiencyModifierCurve() const;

      boost::optional<double> ratedFanPower() const;

      double auxiliaryElectricPower() const;

      bool isAuxiliaryElectricPowerDefaulted() const;

      // boost::optional<WaterStorageTank> waterStorageTank() const;

      std::string inletWaterTemperatureOption() const;

      bool isInletWaterTemperatureOptionDefaulted() const;

      boost::optional<double> autosizedRatedCapacity() const;

      boost::optional<double> autosizedRatedGasUseRate() const;

      virtual void autosize() override;

      virtual void applySizingValues() override;

      //@}
      /** @name Setters */
      //@{

      bool setAvailabilitySchedule(Schedule& schedule);

      void resetAvailabilitySchedule();

      bool setRatedCapacity(boost::optional<double> ratedCapacity);

      void autosizeRatedCapacity();

      bool setRatedGasUseRate(boost::optional<double> ratedGasUseRate);

      void resetRatedGasUseRate();

      void autosizeRatedGasUseRate();

      bool setThermalEfficiency(boost::optional<double> thermalEfficiency);

      void resetThermalEfficiency();

      bool setThermalEfficiencyModifierCurve(const boost::optional<Curve>& curve);

      void resetThermalEfficiencyModifierCurve();

      bool setRatedFanPower(boost::optional<double> ratedFanPower);

      void resetRatedFanPower();

      bool setAuxiliaryElectricPower(boost::optional<double> auxiliaryElectricPower);

      void resetAuxiliaryElectricPower();

      // bool setWaterStorageTank(const boost::optional<WaterStorageTank>& waterStorageTank);

      // void resetWaterStorageTank();

      bool setInletWaterTemperatureOption(boost::optional<std::string> inletWaterTemperatureOption);

      void resetInletWaterTemperatureOption();

      //@}
      /** @name Other */
      //@{

      //@}
     protected:
     private:
      REGISTER_LOGGER("openstudio.model.HumidifierSteamGas");
    };

  }  // namespace detail

}  // namespace model
}  // namespace openstudio

#endif  // MODEL_HUMIDIFIERSTEAMGAS_IMPL_HPP
