#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class DecisionTree {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        if (x[1] <= 82.17666625976562) {
                            if (x[0] <= 40.18499946594238) {
                                return 2;
                            }

                            else {
                                if (x[1] <= 70.35166549682617) {
                                    return 2;
                                }

                                else {
                                    return 3;
                                }
                            }
                        }

                        else {
                            if (x[4] <= 5.034999847412109) {
                                if (x[3] <= 2.0049999952316284) {
                                    if (x[0] <= 39.91999816894531) {
                                        if (x[5] <= 0.11499999836087227) {
                                            return 3;
                                        }

                                        else {
                                            return 2;
                                        }
                                    }

                                    else {
                                        return 3;
                                    }
                                }

                                else {
                                    if (x[0] <= 39.2450008392334) {
                                        if (x[2] <= 3.0) {
                                            return 1;
                                        }

                                        else {
                                            return 3;
                                        }
                                    }

                                    else {
                                        return 1;
                                    }
                                }
                            }

                            else {
                                if (x[0] <= 40.010000228881836) {
                                    if (x[3] <= 3.3399999141693115) {
                                        return 1;
                                    }

                                    else {
                                        if (x[9] <= -0.5750000029802322) {
                                            return 1;
                                        }

                                        else {
                                            return 0;
                                        }
                                    }
                                }

                                else {
                                    return 0;
                                }
                            }
                        }
                    }

                    /**
                    * Predict readable class name
                    */
                    const char* predictLabel(float *x) {
                        return idxToLabel(predict(x));
                    }

                    /**
                    * Convert class idx to readable name
                    */
                    const char* idxToLabel(uint8_t classIdx) {
                        switch (classIdx) {
                            case 0:
                            return "CRITICAL";
                            case 1:
                            return "HIGH";
                            case 2:
                            return "LOW";
                            case 3:
                            return "MODERATE";
                            default:
                            return "Houston we have a problem";
                        }
                    }

                protected:
                };
            }
        }
    }