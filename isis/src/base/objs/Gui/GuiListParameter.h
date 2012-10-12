#ifndef Isis_GuiListParameter_h
#define Isis_GuiListParameter_h

#include <QButtonGroup>

#include "GuiParameter.h"


namespace Isis {
  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class GuiListParameter : public GuiParameter {

      Q_OBJECT

    public:

      GuiListParameter(QGridLayout *grid, UserInterface &ui,
                       int group, int param);
      ~GuiListParameter();

      IString Value();

      void Set(IString newValue);

      virtual std::vector<std::string> Exclusions();

    private:
      QButtonGroup *p_buttonGroup;
  };
};



#endif

