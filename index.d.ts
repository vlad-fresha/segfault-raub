declare module "segfault-raub" {
	/**
	 * Produce a segfault
	 * Issue an actual segfault, accessing some unavailable memory
	*/
	export const causeSegfault: () => void;
	export const causeDivisionInt: () => void;
	export const causeOverflow: () => void;
	export const causeIllegal: () => void;

	// enable / disable signal handlers
	export const setSignal: (signalId: number, value: boolean) => void;
}
