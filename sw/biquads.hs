{-# OPTIONS_GHC -Wall #-}

import Data.List (intercalate)
import System.Environment (getArgs)

-- | Print out some sets of coefficients for a filter bank.
main :: IO ()
main = getArgs >>= \x -> putStrLn $ unlines $ map printCoeffs $ reverse $
                        take (read $ head x) $ drop 2 $
                        repeatF (/2) (11025 :: Double)

-- | Repeatedly apply a function (for creating banks of filters)
repeatF :: (a -> a) -> a -> [a]
repeatF f x = x : repeatF f (f x)

-- | Output the coefficients an audio low-pass filter (ready for C code)
printCoeffs :: (Floating a1, Show a1) => a1 -> String
printCoeffs freq = show (biquadCoeffs LowPass freq 44100 (1/(sqrt 2))) ++ "  /* " ++ show freq ++ " Hz */"

-- | Different types of filters we know about.
data FilterType = LowPass
                | HighPass
                | BandPass
                | Notch
                  deriving (Eq, Show, Read, Enum, Bounded)

-- | A biquad filter
data Biquad a = BQ { bq_a0 :: a
                   , bq_a1 :: a
                   , bq_a2 :: a
                   , bq_b1 :: a
                   , b1_b2 :: a
                   } deriving (Eq, Read)

-- | Derive biquad filter coefficients for a particular filter type,
-- cutoff frequency, sample frequency and quality factor.
biquadCoeffs
  :: (Floating a) =>
     FilterType -> a -> a -> a -> Biquad a
biquadCoeffs filtertype fc fs q' = bqc' filtertype q'
  where
    k = tan $ pi * (fc / fs)
    norm = 1 / (1 + k / q' + k * k)
    bqc' LowPass q = BQ a0 a1 a2 b1 b2
      where
        a0 = k * k * norm
        a1 = 2 * a0
        a2 = a0
        b1 = 2 * (k * k - 1) * norm
        b2 = (1 - k / q + k * k) * norm
    bqc' HighPass q = BQ a0 a1 a2 b1 b2
      where
        a0 = 1 * norm;
        a1 = -2 * a0;
        a2 = a0;
        b1 = 2 * (k * k - 1) * norm;
        b2 = (1 - k / q + k * k) * norm;
    bqc' BandPass q = BQ a0 a1 a2 b1 b2
      where
        a0 = k / q * norm;
        a1 = 0;
        a2 = -a0;
        b1 = 2 * (k * k - 1) * norm;
        b2 = (1 - k / q + k * k) * norm;
    bqc' Notch q = BQ a0 a1 a2 b1 b2
      where
        a0 = (1 + k * k) * norm;
        a1 = 2 * (k * k - 1) * norm;
        a2 = a0;
        b1 = a1;
        b2 = (1 - k / q + k * k) * norm;

-- | Pretty-printing for filter coefficients.
instance Show a => Show (Biquad a) where
  show (BQ a0 a1 a2 b1 b2) = intercalate ", " $ map show $ [a0, a1, a2, b1, b2]
