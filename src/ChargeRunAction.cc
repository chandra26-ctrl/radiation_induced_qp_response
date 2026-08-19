/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "ChargeRunAction.hh"

#include "ChargeConfigManager.hh"
#include "G4Exception.hh"
#include "G4Run.hh"
#include "G4ios.hh"

#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path RunSummaryPath()
{
  const std::filesystem::path hitPath(
      ChargeConfigManager::GetHitOutput().c_str());
  return hitPath.parent_path()
      / ("run_summary_" + hitPath.filename().string());
}

void WriteRunSummary(const G4Run* run, G4int completedEvents,
                     G4bool complete)
{
  const std::filesystem::path summaryPath = RunSummaryPath();
  std::ofstream summary(summaryPath, std::ios::out | std::ios::trunc);
  if (!summary.good()) {
    G4ExceptionDescription message;
    message << "Unable to write run summary " << summaryPath.string();
    G4Exception("ChargeRunAction::WriteRunSummary", "ChargeRun001",
                FatalException, message);
    return;
  }

  summary << "Run ID,Requested Events,Completed Events,Complete\n"
          << run->GetRunID() << ','
          << run->GetNumberOfEventToBeProcessed() << ','
          << completedEvents << ','
          << (complete ? 1 : 0) << '\n';
}

}  // namespace

void ChargeRunAction::BeginOfRunAction(const G4Run* run)
{
  if (!IsMaster()) return;

  // Write an incomplete marker before workers start.  If the executable is
  // interrupted, analysis will see Complete=0 instead of trusting stale data.
  WriteRunSummary(run, 0, false);
}

void ChargeRunAction::EndOfRunAction(const G4Run* run)
{
  if (!IsMaster()) return;

  const G4int requestedEvents = run->GetNumberOfEventToBeProcessed();
  const G4int completedEvents = run->GetNumberOfEvent();
  const G4bool complete = completedEvents == requestedEvents;
  WriteRunSummary(run, completedEvents, complete);

  G4cout << "QP run completion: " << completedEvents << '/'
         << requestedEvents << " events" << G4endl;

  if (!complete) {
    G4ExceptionDescription message;
    message << "Only " << completedEvents << " of " << requestedEvents
            << " requested events completed. Do not normalize the QP data "
               "as a complete run.";
    G4Exception("ChargeRunAction::EndOfRunAction", "ChargeRun002",
                JustWarning, message);
  }
}
