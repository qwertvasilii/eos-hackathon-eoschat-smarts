# CONTRACT FOR eoschat.deal::init

## ACTION NAME: init

### Parameters
Input parameters:

* `initiator` (person initiating deal)
* `executor` (job executor)
* `quantity` (asset quantity of a deal)
* `conditions` (string with additional agreement conditions)
* `multisig` (technical account fot asset freezing)
* `escrow_agent` (mediator of a deal)

### Intent
INTENT. The intention of the initiator to invoke a deal with executor for satisfying the conditions and paying fee.

### Term
TERM. This Contract expires at the approval of a transfer operation to one of the participants.
