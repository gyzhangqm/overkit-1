// Copyright (c) 2018 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_CORE_PUBLIC_DOMAIN_INCLUDED
#define OVK_CORE_PUBLIC_DOMAIN_INCLUDED

#include <ovk/core/ovkAssemblyOptions.h>
#include <ovk/core/ovkConnectivity.h>
#include <ovk/core/ovkGlobal.h>
#include <ovk/core/ovkGrid.h>
#include <ovk/core/ovkExchange.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ovk_domain_params;
typedef struct ovk_domain_params ovk_domain_params;

struct ovk_domain;
typedef struct ovk_domain ovk_domain;

void ovkGetDomainName(const ovk_domain *Domain, char *Name);
void ovkGetDomainDimension(const ovk_domain *Domain, int *NumDims);
void ovkGetDomainComm(const ovk_domain *Domain, MPI_Comm *Comm);
void ovkGetDomainCommSize(const ovk_domain *Domain, int *CommSize);
void ovkGetDomainCommRank(const ovk_domain *Domain, int *CommRank);

void ovkConfigureDomain(ovk_domain *Domain, ovk_domain_config Config);
void ovkGetDomainConfiguration(ovk_domain *Domain, ovk_domain_config *Config);

void ovkGetDomainGridCount(const ovk_domain *Domain, int *NumGrids);
void ovkGetNextAvailableGridID(const ovk_domain *Domain, int *GridID);

void ovkCreateGridLocal(ovk_domain *Domain, int GridID, const ovk_grid_params *Params);
void ovkCreateGridRemote(ovk_domain *Domain, int GridID);
void ovkDestroyGrid(ovk_domain *Domain, int GridID);

bool ovkGridExists(const ovk_domain *Domain, int GridID);
void ovkGetGridInfo(const ovk_domain *Domain, int GridID, const ovk_grid_info **GridInfo);
bool ovkRankHasGrid(const ovk_domain *Domain, int GridID);
void ovkGetGrid(const ovk_domain *Domain, int GridID, const ovk_grid **Grid);
void ovkEditGridLocal(ovk_domain *Domain, int GridID, ovk_grid **Grid);
void ovkEditGridRemote(ovk_domain *Domain, int GridID);
void ovkReleaseGridLocal(ovk_domain *Domain, int GridID, ovk_grid **Grid);
void ovkReleaseGridRemote(ovk_domain *Domain, int GridID);

bool ovkConnectivityExists(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID);
bool ovkRankHasConnectivity(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID);
void ovkGetConnectivity(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  const ovk_connectivity **Connectivity);
void ovkEditConnectivityLocal(ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  ovk_connectivity **Connectivity);
void ovkEditConnectivityRemote(ovk_domain *Domain, int DonorGridID, int ReceiverGridID);
void ovkReleaseConnectivityLocal(ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  ovk_connectivity **Connectivity);
void ovkReleaseConnectivityRemote(ovk_domain *Domain, int DonorGridID, int ReceiverGridID);

bool ovkExchangeExists(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID);
bool ovkRankHasExchange(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID);
void ovkGetExchange(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  const ovk_exchange **Exchange);

void ovkGetLocalDonorCount(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  long long *NumDonors);
void ovkGetLocalReceiverCount(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  long long *NumReceivers);

void ovkAssemble(ovk_domain *Domain, const ovk_assembly_options *Options);

void ovkCollect(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  ovk_data_type DataType, int Count, ovk_collect_op CollectOp, const ovk_range *GridDataRange,
  ovk_array_layout GridDataLayout, const void **GridData, void **DonorData);

void ovkSend(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID, ovk_data_type DataType,
  int Count, const void **DonorData, int Tag, ovk_request **Request);

void ovkReceive(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  ovk_data_type DataType, int Count, void **ReceiverData, int Tag, ovk_request **Request);

void ovkWaitAll(int NumRequests, ovk_request **Requests);

void ovkWaitAny(int NumRequests, ovk_request **Requests, int *Index);

void ovkDisperse(const ovk_domain *Domain, int DonorGridID, int ReceiverGridID,
  ovk_data_type DataType, int Count, ovk_disperse_op DisperseOp, const void **ReceiverData,
  const ovk_range *GridDataRange, ovk_array_layout GridDataLayout, void **GridData);

void ovkCreateDomainParams(ovk_domain_params **Params, int NumDims);
void ovkDestroyDomainParams(ovk_domain_params **Params);

void ovkGetDomainParamName(const ovk_domain_params *Params, char *Name);
void ovkSetDomainParamName(ovk_domain_params *Params, const char *Name);
void ovkGetDomainParamDimension(const ovk_domain_params *Params, int *NumDims);
void ovkGetDomainParamComm(const ovk_domain_params *Params, MPI_Comm *Comm);
void ovkSetDomainParamComm(ovk_domain_params *Params, MPI_Comm Comm);

#ifdef __cplusplus
}
#endif

#endif
