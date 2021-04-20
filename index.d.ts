declare module "segfault-raub" {
	/**
	 * Produce a segfault
	 * Issue an actual segfault, accessing some unavailable memory
	*/
	export const causeSegfault: () => void;
}
