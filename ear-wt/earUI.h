#ifndef _earUI
#define _earUI

#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WString>
#include <Wt/WGroupBox>
#include <Wt/WVBoxLayout>
#include <Wt/WHBoxLayout>
#include <Wt/WAnimation>
#include <Wt/WStringListModel>
#include <Wt/WStandardItemModel>
#include <Wt/WComboBox>
#include <Wt/WSelectionBox>
#include <Wt/WPanel>
#include <Wt/WSlider>
#include <Wt/WTimer>
#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Serializer>
#include <Wt/WSignalMapper>
#include <Wt/WTreeTable>
#include <Wt/WTree>
#include <Wt/WTreeNode>
#include <Wt/WTreeTableNode>
#include <Wt/Chart/WCartesianChart>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <boost/range/adaptor/reversed.hpp>
#include <Wt/WCompositeWidget>
#include <Wt/WBootstrapTheme>

#include "earzmq.h"
#include "filteredStringModel.h"
#include "TimeWidget.h"
#include "fragmentTree.h"
#include <Wt/WLogger>

//#define OLD_WT
class EarUI : public Wt::WApplication
{
public:
  EarUI(const Wt::WEnvironment& env);

  static long start_track_time;
  static long stop_track_time;
  static long time_speed;
  void updateInputs();
private:
  bool posSliderMoving;
  TimeWidget *posText;
  Wt::WLength width = Wt::WLength::Auto; 
  std::vector<MyTreeTableNode*> fragment_set;
  void clicked(Wt::WPushButton* source );
  Wt::WTreeTable *markerTree;
  MyTreeTableNode *addNode(MyTreeTableNode *parent, Wt::WString name, const long start, const long stop );
  Wt::Json::Value saveFragments(MyTreeTableNode *root);
  
  Wt::WPushButton *playPauseButton;

  std::map<std::string, Wt::WText*> inputTexts;
  std::map<std::string, Wt::WSlider*> inputSliders;
  long current_track_time( zmq::socket_t *socket  =0  );
  Wt::WSlider *posSlider;
  Wt::WSlider *beforeSlider;

  #ifdef  PLOT
  void loadWaveform();
  Wt::WStandardItemModel *waveformModel;
  Wt::Chart::WCartesianChart *waveformChart;
  Wt::WText *chartText;
  Wt::WTimer *waveformTimer;
  #endif

  int ui_track_idx;
  int max_tags;
};



#include "trackSearchContainer.h" //Include after class definition because it uses the class

#endif
