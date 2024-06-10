#include <tcl/tcl.h>

#include "ord/OpenRoad.hh"

namespace sta {
}  // namespace sta

class dbVerilogNetwork;
class Logger;
class dbSta;
class dbBlock;
char** cmd_argv;
int cmd_argc;
const char* log_filename = "";
const char* metrics_filename = "";

static const char* init_filename = ".openroad";

int ord::tclAppInit(Tcl_Interp* interp)
{
  return -1;
}
int Openroad_swig_Init(Tcl_Interp* interp)
{
  return -1;
}
int Odbtcl_Init(Tcl_Interp* interp)
{
}
int Upf_Init(Tcl_Interp* interp)
{
}

namespace sta {
const char* openroad_swig_tcl_inits;
const char* upf_tcl_inits;
}  // namespace sta
namespace ord {

utl::Logger* getLogger()
{
  return nullptr;
}
/*
Logger* makeLogger(const char* log_filename, const char* metrics_filename)
{
}
*/

dbSta* makeDbSta()
{
  return nullptr;
}
/*
dbVerilogNetwork* makeDbVerilogNetwork()
{
}
*/
rsz::Resizer* makeResizer()
{
  return nullptr;
}

ppl::IOPlacer* makeIoplacer()
{
  return nullptr;
}
dpl::Opendp* makeOpendp()
{
  return nullptr;
}
dpo::Optdp* makeOptdp()
{
  return nullptr;
}
fin::Finale* makeFinale()
{
  return nullptr;
}
grt::GlobalRouter* makeGlobalRouter()
{
  return nullptr;
}
rmp::Restructure* makeRestructure()
{
  return nullptr;
}
cts::TritonCTS* makeTritonCts()
{
  return nullptr;
}
tap::Tapcell* makeTapcell()
{
  return nullptr;
}
mpl::MacroPlacer* makeMacroPlacer()
{
  return nullptr;
}
rcx::Ext* makeOpenRCX()
{
  return nullptr;
}
triton_route::TritonRoute* makeTritonRoute()
{
  return nullptr;
}
gpl::Replace* makeReplace()
{
  return nullptr;
}
psm::PDNSim* makePDNSim()
{
  return nullptr;
}
ant::AntennaChecker* makeAntennaChecker()
{
  return nullptr;
}
pdn::PdnGen* makePdnGen()
{
  return nullptr;
}
pad::ICeWall* makeICeWall()
{
  return nullptr;
}
dst::Distributed* makeDistributed()
{
  return nullptr;
}
stt::SteinerTreeBuilder* makeSteinerTreeBuilder()
{
  return nullptr;
}
dft::Dft* makeDft()
{
  return nullptr;
}
/*
mdm::MultiDieManager* makeMultiDieManager()
{
}
*/

void evalTclInit(Tcl_Interp* interp, const char* inits[])
{
}
void initLogger(Logger* logger, Tcl_Interp* tcl_interp)
{
}
/*
void initGui(OpenRoad* openroad)
{
}
*/

void initInitFloorplan(OpenRoad* openroad)
{
}
void initDbSta(OpenRoad* openroad)
{
}
void initResizer(OpenRoad* openroad)
{
}
/*
void initDbVerilogNetwork(ord::OpenRoad* openroad)
{
}
*/
void initIoplacer(OpenRoad* openroad)
{
}
void initReplace(OpenRoad* openroad)
{
}
void initOpendp(OpenRoad* openroad)
{
}
void initOptdp(OpenRoad* openroad)
{
}
void initFinale(OpenRoad* openroad)
{
}
void initGlobalRouter(OpenRoad* openroad)
{
}
void initTritonCts(OpenRoad* openroad)
{
}
void initTapcell(OpenRoad* openroad)
{
}
void initMacroPlacer(OpenRoad* openroad)
{
}
void initOpenRCX(OpenRoad* openroad)
{
}
void initICeWall(OpenRoad* openroad)
{
}
void initRestructure(OpenRoad* openroad)
{
}
void initTritonRoute(OpenRoad* openroad)
{
}
void initPDNSim(OpenRoad* openroad)
{
}
void initAntennaChecker(OpenRoad* openroad)
{
}
void initPdnGen(OpenRoad* openroad)
{
}
void initDistributed(OpenRoad* openroad)
{
}
void initSteinerTreeBuilder(OpenRoad* openroad)
{
}
/*
void initMultiDieManager(OpenRoad* openroad)
{
}
*/

/*
void deleteDbVerilogNetwork(dbVerilogNetwork* verilog_network)
{
}
*/
void deleteIoplacer(ppl::IOPlacer* ioplacer)
{
}
void deleteResizer(rsz::Resizer* resizer)
{
}
void deleteOpendp(dpl::Opendp* opendp)
{
}
void deleteOptdp(dpo::Optdp* optdp)
{
}
void deleteGlobalRouter(grt::GlobalRouter* global_router)
{
}
void deleteRestructure(rmp::Restructure* restructure)
{
}
void deleteTritonCts(cts::TritonCTS* tritoncts)
{
}
void deleteTapcell(tap::Tapcell* tapcell)
{
}
void deleteMacroPlacer(mpl::MacroPlacer* macro_placer)
{
}
void deleteMacroPlacer2(mpl2::MacroPlacer2* macro_placer)
{
}
void deleteOpenRCX(rcx::Ext* extractor)
{
}
void deleteTritonRoute(triton_route::TritonRoute* router)
{
}
void deleteReplace(gpl::Replace* replace)
{
}
void deleteFinale(fin::Finale* finale)
{
}
void deleteAntennaChecker(ant::AntennaChecker* antenna_checker)
{
}
void deletePdnGen(pdn::PdnGen* pdngen)
{
}
void deleteICeWall(pad::ICeWall* icewall)
{
}
void deleteDistributed(dst::Distributed* dstr)
{
}
void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder)
{
}
}  // namespace ord

namespace dft {
dft::Dft* makeDft()
{
  return nullptr;
}
void initDft(ord::OpenRoad* openroad)
{
}
void deleteDft(dft::Dft* dft)
{
}
}  // namespace dft
namespace ifp {
class InitFloorplan
{
 public:
  InitFloorplan(odb::dbBlock* block, Logger* logger, sta::dbNetwork* network) {}
};
}  // namespace ifp